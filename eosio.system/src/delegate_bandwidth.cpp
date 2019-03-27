/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio.system/eosio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.h>
#include <eosiolib/transaction.hpp>

#include <eosio.token/eosio.token.hpp>


#include <cmath>
#include <map>

namespace eosiosystem {
   using eosio::asset;
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::print;
   using eosio::permission_level;
   using eosio::time_point_sec;
   using std::map;
   using std::pair;

   static constexpr uint32_t refund_delay_sec = 3*24*3600;
   static constexpr int64_t  ram_gift_bytes = 0;

   struct [[eosio::table, eosio::contract("eosio.system")]] user_resources {
      name          owner;
      asset         net_weight;
      asset         cpu_weight;
      asset         ram_stake;
      int64_t       ram_bytes = 0;

      uint64_t primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( user_resources, (owner)(net_weight)(cpu_weight)(ram_stake)(ram_bytes) )
   };


   /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct [[eosio::table, eosio::contract("eosio.system")]] delegated_bandwidth {
      name          from;
      name          to;
      asset         net_weight;
      asset         cpu_weight;

      uint64_t  primary_key()const { return to.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_bandwidth, (from)(to)(net_weight)(cpu_weight) )

   };

      /**
    *  Every user 'from' has a scope/table that uses every receipient 'to' as the primary key.
    */
   struct [[eosio::table, eosio::contract("eosio.system")]] delegated_ram {
      name          from;
      name          to;
      asset         ram_stake;
      int64_t       ram_bytes = 0;

      uint64_t  primary_key()const { return to.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( delegated_ram, (from)(to)(ram_stake)(ram_bytes) )

   };

   struct [[eosio::table, eosio::contract("eosio.system")]] refund_request {
      name            owner;
      time_point_sec  request_time;
      eosio::asset    net_amount;
      eosio::asset    cpu_amount;
      eosio::asset    ram_amount;
      uint64_t        ram_bytes;


      uint64_t  primary_key()const { return owner.value; }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( refund_request, (owner)(request_time)(net_amount)(cpu_amount)(ram_amount)(ram_bytes) )
   };

   /**
    *  These tables are designed to be constructed in the scope of the relevant user, this
    *  facilitates simpler API for per-user queries
    */
   typedef eosio::multi_index< "userres"_n, user_resources >      user_resources_table;
   typedef eosio::multi_index< "delband"_n, delegated_bandwidth > del_bandwidth_table;
   typedef eosio::multi_index< "delram"_n, delegated_ram >        del_ram_table;
   typedef eosio::multi_index< "refunds"_n, refund_request >      refunds_table;



   /**
    *  This action will buy an exact amount of ram and bill the payer the current market price.
    */
   void system_contract::buyrambytes( name payer, name receiver, uint32_t bytes ) {
      eosio_assert( payer == "worbli.admin"_n || payer == _self , "only worbli.admin or eosio can purchase RAM" );
      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const uint64_t token_precision = token_supply.symbol.precision();
      const uint64_t bytes_per_token = uint64_t((_gstate.max_ram_size / (double)token_supply.amount) * pow(10,token_precision));
      
      auto eosout = int64_t((bytes * pow(10,token_precision)) / bytes_per_token);

      buyram( payer, receiver, asset(eosout, core_symbol()) );
   }


  void system_contract::buyram( name payer, name receiver, asset quant ) {
      require_auth( payer );
      eosio_assert( payer == "worbli.admin"_n || _self , "only worbli.admin or eosio can purchase RAM" );
      eosio_assert( quant.symbol == core_symbol(), "must buy ram with core token" );
      eosio_assert( quant.amount > 0, "must purchase a positive amount" );

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const uint64_t token_precision = token_supply.symbol.precision();
      const uint64_t bytes_per_token = uint64_t(((_gstate.max_ram_size * pow(10,token_precision)) / (double)token_supply.amount) );
      
      uint64_t bytes_out = uint64_t(bytes_per_token * quant.amount / pow(10,token_precision));
    
      _gstate.total_ram_bytes_reserved += uint64_t(bytes_out);
      _gstate.total_ram_stake          += quant.amount;

        user_resources_table  userres( _self, receiver.value );
        auto res_itr = userres.find( receiver.value );

        if( res_itr ==  userres.end() ) {
            res_itr = userres.emplace( receiver, [&]( auto& res ) {
                  res.owner = receiver;
                  res.net_weight = asset( 0, core_symbol() );
                  res.cpu_weight = asset( 0, core_symbol() );
                  res.ram_bytes = bytes_out;
                  res.ram_stake = quant;
                  });
        } else {
            userres.modify( res_itr, receiver, [&]( auto& res ) {
                  res.ram_bytes += bytes_out;
                  res.ram_stake += quant;
                  });
        }
         
        auto voter_itr = _voters.find( res_itr->owner.value );
        if( voter_itr == _voters.end() || !has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed ) ) {
           int64_t ram_bytes, net, cpu;
           get_resource_limits( res_itr->owner.value, &ram_bytes, &net, &cpu );
           set_resource_limits( res_itr->owner.value, res_itr->ram_bytes + ram_gift_bytes, net, cpu );
        }
 
      // create refund or update from existing refund
      if ( "eosio.stake"_n != receiver ) { //for eosio both transfer and refund make no sense
         refunds_table refunds_tbl( _self, receiver.value );
         auto req = refunds_tbl.find( receiver.value );

         //create/update/delete refund
         auto ram_balance = quant;
         bool need_deferred_trx = false;


         if ( req != refunds_tbl.end() ) { //need to update refund
            refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
                r.request_time = current_time_point();
                r.ram_amount -= ram_balance;
                r.ram_bytes -= bytes_out;
                if ( r.ram_amount.amount < 0 ) {
                   ram_balance = -r.ram_amount;
                   r.ram_amount.amount = 0;
                   r.ram_bytes = 0;
                } else {
                   ram_balance.amount = 0;
                }                  
             });

             eosio_assert( 0 <= req->ram_amount.amount, "negative ram refund amount" ); //should never happen

             if ( req->net_amount.amount == 0 && req->cpu_amount.amount == 0 && req->ram_bytes == 0 &&
                req->ram_amount.amount == 0 ) {
                refunds_tbl.erase( req );
                need_deferred_trx = false;
             } else {
                need_deferred_trx = true;
             }
         } 


         if ( need_deferred_trx ) {
            eosio::transaction out;
            out.actions.emplace_back( permission_level{ receiver, active_permission }, _self, "refund"_n, receiver );
            out.delay_sec = refund_delay_sec;
            cancel_deferred( receiver.value ); // TODO: Remove this line when replacing deferred trxs is fixed
            out.send( receiver.value, receiver, true );
         } else {
            cancel_deferred( receiver.value );
         }

         auto transfer_amount = ram_balance;
         if ( 0 < transfer_amount.amount ) {
            INLINE_ACTION_SENDER(eosio::token, transfer)( token_account, {payer, active_permission},
               { payer, stake_account, asset(transfer_amount), std::string("stake ram") } );
         }
      }
  }

  /**
    *  The system contract now buys and sells RAM allocations at prevailing market prices.
    *  This may result in traders buying RAM today in anticipation of potential shortages
    *  tomorrow. Overall this will result in the market balancing the supply and demand
    *  for RAM over time.
    */
   void system_contract::sellram( name account, int64_t bytes ) {
      require_auth( account );
      eosio_assert( bytes > 0, "cannot sell negative byte" );
      int64_t delegated_ram_bytes = 0;

      // lookup delegated from worbli.admin
      del_ram_table  delram_admin( _self, "worbli.admin"_n.value );
      auto ram_itr = delram_admin.find( account.value );

      if(ram_itr != delram_admin.end())
            delegated_ram_bytes += ram_itr->ram_bytes;

      // lookup delegated from eosio
      del_ram_table  delram_eosio( _self, _self.value );
      ram_itr = delram_eosio.find( account.value );

      if(ram_itr != delram_eosio.end())
            delegated_ram_bytes += ram_itr->ram_bytes;

      user_resources_table  userres( _self, account.value );
      auto res_itr = userres.find( account.value );
      eosio_assert( res_itr != userres.end(), "no resource row" );
      eosio_assert( res_itr->ram_bytes - delegated_ram_bytes >= bytes, "insufficient quota" );
       
      asset tokens_out;
      int64_t ram_bytes = res_itr->ram_bytes;
      float_t token_per_bytes = res_itr->ram_stake.amount / (float_t)(ram_bytes);
      int64_t tokens = token_per_bytes * bytes;

      tokens_out = asset(tokens, core_symbol());

      eosio_assert( tokens_out.amount > 1, "token amount received from selling ram is too low" );

      _gstate.total_ram_bytes_reserved -= static_cast<decltype(_gstate.total_ram_bytes_reserved)>(bytes); // bytes > 0 is asserted above
      _gstate.total_ram_stake          -= tokens_out.amount;

      //// this shouldn't happen, but just in case it does we should prevent it
      eosio_assert( _gstate.total_ram_stake >= 0, "error, attempt to unstake more tokens than previously staked" );

      userres.modify( res_itr, account, [&]( auto& res ) {
          res.ram_bytes -= bytes;
          res.ram_stake -= tokens_out;
      });

      auto voter_itr = _voters.find( res_itr->owner.value );
      if( voter_itr == _voters.end() || !has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed ) ) {
         int64_t ram_bytes, net, cpu;
         get_resource_limits( res_itr->owner.value, &ram_bytes, &net, &cpu );
         set_resource_limits( res_itr->owner.value, res_itr->ram_bytes + ram_gift_bytes, net, cpu );
         
      }
      
            // create refund or update from existing refund
      if ( "eosio.stake"_n != account ) { //for eosio both transfer and refund make no sense
         refunds_table refunds_tbl( _self, account.value );
         auto req = refunds_tbl.find( account.value );

         //create/update/delete refund
         auto ram_balance = tokens_out;
         bool need_deferred_trx = false;

            if ( req != refunds_tbl.end() ) { //need to update refund
               refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
                  r.request_time = current_time_point();
                  r.ram_amount += ram_balance;   
                  r.ram_bytes += bytes;               
               });

               if ( req->net_amount.amount == 0 && req->cpu_amount.amount == 0 && req->ram_bytes == 0 &&
                  req->ram_amount.amount == 0 ) {
                  refunds_tbl.erase( req );
                  need_deferred_trx = false;
               } else {
                  need_deferred_trx = true;
               }

            } else { //need to create refund
               refunds_tbl.emplace( account, [&]( refund_request& r ) {
                  r.owner = account;
                  r.ram_amount = ram_balance;
                  r.ram_bytes = bytes;
                  r.cpu_amount = asset( 0, core_symbol() );
                  r.net_amount = asset( 0, core_symbol() );
                  r.request_time = current_time_point();
               });
               need_deferred_trx = true;
            } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl      

         if ( need_deferred_trx ) {
            eosio::transaction out;
            out.actions.emplace_back( permission_level{ account, active_permission }, _self, "refund"_n, account );
            out.delay_sec = refund_delay_sec;
            cancel_deferred( account.value ); // TODO: Remove this line when replacing deferred trxs is fixed
            out.send( account.value, account, true );
         } else {
            cancel_deferred( account.value );
         }
      }
      // need to update voting power    
   }

   void system_contract::changebw( name from, name receiver,
                                   const asset stake_net_delta, const asset stake_cpu_delta, bool transfer )
   {
      require_auth( from );
      eosio_assert( stake_net_delta.amount != 0 || stake_cpu_delta.amount != 0, "should stake non-zero amount" );
      eosio_assert( std::abs( (stake_net_delta + stake_cpu_delta).amount )
                     >= std::max( std::abs( stake_net_delta.amount ), std::abs( stake_cpu_delta.amount ) ),
                    "net and cpu deltas cannot be opposite signs" );

      name source_stake_from = from;
      if ( transfer ) {
         from = receiver;
      }

      // update stake delegated from "from" to "receiver"
      {
         del_bandwidth_table     del_tbl( _self, from.value );
         auto itr = del_tbl.find( receiver.value );
         if( itr == del_tbl.end() ) {
            itr = del_tbl.emplace( from, [&]( auto& dbo ){
                  dbo.from          = from;
                  dbo.to            = receiver;
                  dbo.net_weight    = stake_net_delta;
                  dbo.cpu_weight    = stake_cpu_delta;
               });
         }
         else {
            del_tbl.modify( itr, same_payer, [&]( auto& dbo ){
                  dbo.net_weight    += stake_net_delta;
                  dbo.cpu_weight    += stake_cpu_delta;
               });
         }
         eosio_assert( 0 <= itr->net_weight.amount, "insufficient staked net bandwidth" );
         eosio_assert( 0 <= itr->cpu_weight.amount, "insufficient staked cpu bandwidth" );
         if ( itr->net_weight.amount == 0 && itr->cpu_weight.amount == 0 ) {
            del_tbl.erase( itr );
         }
      } // itr can be invalid, should go out of scope

      // update totals of "receiver"
      {
         user_resources_table   totals_tbl( _self, receiver.value );
         auto tot_itr = totals_tbl.find( receiver.value );
         if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
                  tot.owner = receiver;
                  tot.net_weight    = stake_net_delta;
                  tot.cpu_weight    = stake_cpu_delta;
                  tot.ram_stake     = asset( 0, core_symbol() );
               });
         } else {
            totals_tbl.modify( tot_itr, from == receiver ? from : same_payer, [&]( auto& tot ) {
                  tot.net_weight    += stake_net_delta;
                  tot.cpu_weight    += stake_cpu_delta;
               });
         }
         eosio_assert( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
         eosio_assert( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

         {
            bool ram_managed = false;
            bool net_managed = false;
            bool cpu_managed = false;

            auto voter_itr = _voters.find( receiver.value );
            if( voter_itr != _voters.end() ) {
               ram_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::ram_managed );
               net_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::net_managed );
               cpu_managed = has_field( voter_itr->flags1, voter_info::flags1_fields::cpu_managed );
            }

            if( !(net_managed && cpu_managed) ) {
               int64_t ram_bytes, net, cpu;
               get_resource_limits( receiver.value, &ram_bytes, &net, &cpu );

               set_resource_limits( receiver.value,
                                    ram_managed ? ram_bytes : std::max( tot_itr->ram_bytes + ram_gift_bytes, ram_bytes ),
                                    net_managed ? net : tot_itr->net_weight.amount,
                                    cpu_managed ? cpu : tot_itr->cpu_weight.amount );
            }
         }

         if ( tot_itr->net_weight.amount == 0 && tot_itr->cpu_weight.amount == 0  && tot_itr->ram_bytes == 0 && 
              tot_itr->ram_stake.amount == 0) {
            totals_tbl.erase( tot_itr );
         }
      } // tot_itr can be invalid, should go out of scope

      // create refund or update from existing refund
      if ( stake_account != source_stake_from ) { //for eosio both transfer and refund make no sense
         refunds_table refunds_tbl( _self, from.value );
         auto req = refunds_tbl.find( from.value );

         //create/update/delete refund
         auto net_balance = stake_net_delta;
         auto cpu_balance = stake_cpu_delta;
         bool need_deferred_trx = false;


         // net and cpu are same sign by assertions in delegatebw and undelegatebw
         // redundant assertion also at start of changebw to protect against misuse of changebw
         bool is_undelegating = (net_balance.amount + cpu_balance.amount ) < 0;
         bool is_delegating_to_self = (!transfer && from == receiver);

         if( is_delegating_to_self || is_undelegating ) {
            if ( req != refunds_tbl.end() ) { //need to update refund
               refunds_tbl.modify( req, same_payer, [&]( refund_request& r ) {
                  if ( net_balance.amount < 0 || cpu_balance.amount < 0 ) {
                     r.request_time = current_time_point();
                  }
                  r.net_amount -= net_balance;
                  if ( r.net_amount.amount < 0 ) {
                     net_balance = -r.net_amount;
                     r.net_amount.amount = 0;
                  } else {
                     net_balance.amount = 0;
                  }
                  r.cpu_amount -= cpu_balance;
                  if ( r.cpu_amount.amount < 0 ){
                     cpu_balance = -r.cpu_amount;
                     r.cpu_amount.amount = 0;
                  } else {
                     cpu_balance.amount = 0;
                  }
               });

               eosio_assert( 0 <= req->net_amount.amount, "negative net refund amount" ); //should never happen
               eosio_assert( 0 <= req->cpu_amount.amount, "negative cpu refund amount" ); //should never happen

               if ( req->net_amount.amount == 0 && req->cpu_amount.amount == 0 && req->ram_bytes == 0 &&
                  req->ram_amount.amount == 0) {
                  refunds_tbl.erase( req );
                  need_deferred_trx = false;
               } else {
                  need_deferred_trx = true;
               }
            } else if ( net_balance.amount < 0 || cpu_balance.amount < 0 ) { //need to create refund
               refunds_tbl.emplace( from, [&]( refund_request& r ) {
                  r.owner = from;
                  if ( net_balance.amount < 0 ) {
                     r.net_amount = -net_balance;
                     net_balance.amount = 0;
                  } else {
                     r.net_amount = asset( 0, core_symbol() );
                  }
                  if ( cpu_balance.amount < 0 ) {
                     r.cpu_amount = -cpu_balance;
                     cpu_balance.amount = 0;
                  } else {
                     r.cpu_amount = asset( 0, core_symbol() );
                  }
                  r.ram_amount = asset( 0, core_symbol() );
                  r.ram_bytes = 0;
                  r.request_time = current_time_point();
               });
               need_deferred_trx = true;
            } // else stake increase requested with no existing row in refunds_tbl -> nothing to do with refunds_tbl
         } /// end if is_delegating_to_self || is_undelegating

         if ( need_deferred_trx ) {
            eosio::transaction out;
            out.actions.emplace_back( permission_level{from, active_permission},
                                      _self, "refund"_n,
                                      from
            );
            out.delay_sec = refund_delay_sec;
            cancel_deferred( from.value ); // TODO: Remove this line when replacing deferred trxs is fixed
            out.send( from.value, from, true );
         } else {
            cancel_deferred( from.value );
         }

         auto transfer_amount = net_balance + cpu_balance;
         if ( 0 < transfer_amount.amount ) {
            INLINE_ACTION_SENDER(eosio::token, transfer)(
               token_account, { {source_stake_from, active_permission} },
               { source_stake_from, stake_account, asset(transfer_amount), std::string("stake bandwidth") }
            );
         }
      }

   }

   void system_contract::delegatebw( name from, name receiver,
                                     asset stake_net_quantity,
                                     asset stake_cpu_quantity, bool transfer )
   {
      asset zero_asset( 0, core_symbol() );
      eosio_assert( stake_cpu_quantity >= zero_asset, "must stake a positive amount" );
      eosio_assert( stake_net_quantity >= zero_asset, "must stake a positive amount" );
      eosio_assert( stake_net_quantity.amount + stake_cpu_quantity.amount > 0, "must stake a positive amount" );
      eosio_assert( !transfer || from != receiver, "cannot use transfer flag if delegating to self" );

      changebw( from, receiver, stake_net_quantity, stake_cpu_quantity, transfer);
   } // delegatebw

   void system_contract::undelegatebw( name from, name receiver,
                                       asset unstake_net_quantity, asset unstake_cpu_quantity )
   {
      asset zero_asset( 0, core_symbol() );
      eosio_assert( unstake_cpu_quantity >= zero_asset, "must unstake a positive amount" );
      eosio_assert( unstake_net_quantity >= zero_asset, "must unstake a positive amount" );
      eosio_assert( unstake_cpu_quantity.amount + unstake_net_quantity.amount > 0, "must unstake a positive amount" );
      // eosio_assert( _gstate.total_activated_stake >= min_activated_stake,
      //              "cannot undelegate bandwidth until the chain is activated (at least 15% of all tokens participate in voting)" );

      changebw( from, receiver, -unstake_net_quantity, -unstake_cpu_quantity, false);
   } // undelegatebw

   void system_contract::delegateram( name from, name receiver,
                                     int64_t bytes )
   {
      eosio_assert( from == "worbli.admin"_n || from == _self, "action restricted to worbli.admin and create accounts" );
      require_auth( from );
      eosio_assert( bytes >= 0, "must delegate a positive amount" );

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const uint64_t token_precision = token_supply.symbol.precision();
      const uint64_t bytes_per_token = uint64_t((_gstate.max_ram_size / (double)token_supply.amount) * pow(10,token_precision));
      auto amount = int64_t((bytes * pow(10,token_precision)) / bytes_per_token);

      require_auth( from );
      eosio_assert( bytes != 0, "should stake non-zero amount" );


      // update stake delegated from "from" to "receiver"
      {
         del_ram_table     del_tbl( _self, from.value);
         auto itr = del_tbl.find( receiver.value );
         if( itr == del_tbl.end() ) {
            itr = del_tbl.emplace( from, [&]( auto& dbo ){
                  dbo.from          = from;
                  dbo.to            = receiver;
                  dbo.ram_stake     = asset(amount, core_symbol());
                  dbo.ram_bytes     = bytes;
               });
         }
         else {
            del_tbl.modify( itr, same_payer, [&]( auto& dbo ){
                  dbo.ram_stake    += asset(amount, core_symbol());
                  dbo.ram_bytes    += bytes;
               });
         }

      } // itr can be invalid, should go out of scope

      // update totals of "receiver"
      {
         user_resources_table   totals_tbl( _self, receiver.value );
         auto tot_itr = totals_tbl.find( receiver.value );
         if( tot_itr ==  totals_tbl.end() ) {
            tot_itr = totals_tbl.emplace( from, [&]( auto& tot ) {
                  tot.owner = receiver;
                  tot.ram_stake    = asset(amount, core_symbol());
                  tot.ram_bytes    = bytes;
               });
         } else {
            totals_tbl.modify( tot_itr, same_payer, [&]( auto& tot ) {
                  tot.ram_stake    += asset(amount, core_symbol());
                  tot.ram_bytes    += bytes;
               });
         }
         eosio_assert( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
         eosio_assert( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

         set_resource_limits( receiver.value, tot_itr->ram_bytes, tot_itr->net_weight.amount, tot_itr->cpu_weight.amount );

         if ( tot_itr->net_weight.amount == 0 && tot_itr->cpu_weight.amount == 0  && tot_itr->ram_bytes == 0 && 
              tot_itr->ram_stake.amount == 0 ) {
            totals_tbl.erase( tot_itr );
         }
      } // tot_itr can be invalid, should go out of scope

   } // delegateram


   void system_contract::refund( const name owner ) {
      require_auth( owner );

      refunds_table refunds_tbl( _self, owner.value );
      auto req = refunds_tbl.find( owner.value );
      eosio_assert( req != refunds_tbl.end(), "refund request not found" );
      eosio_assert( req->request_time + seconds(refund_delay_sec) <= current_time_point(),
                    "refund is not available yet" );

      INLINE_ACTION_SENDER(eosio::token, transfer)(
         token_account, { {stake_account, active_permission}, {req->owner, active_permission} },
         { stake_account, req->owner, req->net_amount + req->cpu_amount, std::string("unstake") }
      );

      refunds_tbl.erase( req );
   }


} //namespace eosiosystem
