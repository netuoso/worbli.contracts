#include <eosio.system/worbli.prov.common.hpp>
#include <cmath>

namespace eosiosystem {

   void system_contract::delegateram( name from, name receiver,
                                     int64_t bytes )
   {
      check( from == "worbli.admin"_n || from == _self, "action restricted to worbli.admin and create accounts" );
      require_auth( from );
      check( bytes >= 0, "must delegate a positive amount" );

      const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
      const uint64_t token_precision = token_supply.symbol.precision();
      const uint64_t bytes_per_token = uint64_t((_gstate.max_ram_size / (double)token_supply.amount) * pow(10,token_precision));
      auto amount = int64_t((bytes * pow(10,token_precision)) / bytes_per_token);

      require_auth( from );
      check( bytes != 0, "should stake non-zero amount" );


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
         check( 0 <= tot_itr->net_weight.amount, "insufficient staked total net bandwidth" );
         check( 0 <= tot_itr->cpu_weight.amount, "insufficient staked total cpu bandwidth" );

         set_resource_limits( receiver, tot_itr->ram_bytes, tot_itr->net_weight.amount, tot_itr->cpu_weight.amount );

         if ( tot_itr->net_weight.amount == 0 && tot_itr->cpu_weight.amount == 0  && tot_itr->ram_bytes == 0 && 
              tot_itr->ram_stake.amount == 0 ) {
            totals_tbl.erase( tot_itr );
         }
      } // tot_itr can be invalid, should go out of scope

   } // delegateram

   void system_contract::regproducer( const name& producer, const eosio::public_key& producer_key, const std::string& url, uint16_t location ) {
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      require_auth( producer );

      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "account is not registered as a producer" );
      check( prod->active(), "account is not an active producer" );

      if ( prod != _producers.end() ) {
         _producers.modify( prod, producer, [&]( producer_info& info ){
            info.producer_key = producer_key;
            info.url          = url;
            info.location     = location;
         });
      }
   }

   void system_contract::unregprod( const name& producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer.value, "producer not found" );
      _producers.modify( prod, same_payer, [&]( producer_info& info ){
         info.producer_key = eosio::public_key();
         info.unpaid_blocks = 0;
      });
   }

   void system_contract::update_elected_producers( const block_timestamp& block_time ) {
      _gstate.last_producer_schedule_update = block_time;

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_producers;

      for( const auto& p : _producers ) {
        if( p.producer_key != eosio::public_key() )
            top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{p.owner, p.producer_key}, p.location}) );
      }
      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());
      for( const auto& item : top_producers )
         producers.push_back(item.first);

      if( set_proposed_producers( producers ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }
   }


   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of eosio to register
    *
    */
   void system_contract::addprod( const name producer ) {
      check( producer != "worbli.admin"_n, "producer should not be worbli.admin" );
      check( producer != "eosio"_n, "producer should not be eosio" );
      require_auth( "worbli.admin"_n );

      auto prod = _producers.find( producer.value );

      check( prod == _producers.end(), "account already registered as a producer" );

      _producers.emplace( producer, [&]( producer_info& info ){
            info.owner         = producer;
            info.is_active     = false;
      });
   }

   /**
    *  This method will promote a producer to active 
    *
    *  @pre producer is in the producers table
    *
    */
   void system_contract::promoteprod( const name producer ) {
      require_auth( "worbli.admin"_n );

      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "producer has not been registered yet" );

      _producers.modify( prod, same_payer, [&]( auto& p ) {
         p.is_active     = true;
      });
   }

   /**
    *  This method will demote a producer to reserver 
    *
    *  @pre producer is in the producers table
    *
    */
   void system_contract::demoteprod( const name producer ) {
      require_auth( "worbli.admin"_n );

      auto prod = _producers.find( producer.value );
      check( prod != _producers.end(), "producer has not been registered yet" );

      _producers.modify( prod, same_payer, [&]( auto& p ) {
            p.deactivate();
            p.unpaid_blocks = 0;
      });
   }

   /**
    *  This method will update the key and active flag to true in producer_info object for 'producer'
    *
    *  @pre producer is already registered
    *  @pre authority of producer to update
    *
    */
   void system_contract::togglesched( bool is_active ) {
      require_auth( _self );
      _gstate.is_producer_schedule_active = is_active;

   }

   void system_contract::setusagelvl( uint8_t new_level ) {
      require_auth( "worbli.admin"_n );

      check( _gstate.network_usage_level < new_level, "usage level may only be increased" ); 
      check( new_level <= 100, "usage level cannot excced 100" );
      check( new_level > 0, "usage level cannot be negative" );

      _gstate.network_usage_level = new_level;
      _global.set( _gstate, _self );
   }

    void system_contract::setwparams(uint64_t max_subaccounts) {
      require_auth( "worbli.admin"_n );
      _wstate.max_subaccounts = max_subaccounts;
    }

    void system_contract::cleanup() {
      require_auth( "worbli.admin"_n );
      producers_new_table   producers_new( _self, _self.value );
      for (auto itr = producers_new.cbegin(); itr != producers_new.cend(); itr++) {
         _producers.emplace( itr->owner, [&]( auto& p ) {
            p.owner = itr->owner;
            p.total_votes = itr->total_votes;
            p.producer_key = itr->producer_key;
            p.is_active = itr->is_active;
            p.url = itr->url;
            p.unpaid_blocks = itr->unpaid_blocks;
            p.last_claim_time = itr->last_claim_time;
            p.location = itr->location;
         });
         producers_new.erase(itr);
      }
    }

   // worbli additions
    void native::can_create_subaccount(name creator) {

        if(creator == "worbli.admin"_n || creator == _self) return;

        // no validation if worbli.prov account does not exist.
        if(!is_account(provider_account)) return;

         // TODO: make condition name an enum
         // TODO: add optional comparator to condition >, <, = etc...
         std::vector<worblisystem::condition> conditions {
            worblisystem::condition{provider_account, "identity"_n, {"true"}}
         };

         auto result = worblisystem::validate(creator, conditions);
         check(result.empty(), creator.to_string() + " failed identity check");

         /**
          * if null set max_subaccount = 0
          **/
         std::optional<int64_t> opt = worblisystem::getint(provider_account, creator, "maxsubacct"_n);
         int64_t max_subaccounts = opt ? *opt : 0;

        worbli_params_singleton worbliparams(_self, _self.value);
        worbli_params wstate = worbliparams.exists() ? worbliparams.get() : worbli_params{0};

        subaccount_table subaccounts(_self, creator.value);
        auto sub_count = std::distance(subaccounts.cbegin(),subaccounts.cend());

        max_subaccounts = max_subaccounts < 0 ? wstate.max_subaccounts : max_subaccounts;
        check( max_subaccounts > sub_count, "subaccount limit reached" );

    }

    void native::create_account_records(name account, name parent, int64_t max_subaccounts) {

        subaccount_table subaccounts(_self, parent.value);

        auto sub_itr = subaccounts.find(account.value);
        if(sub_itr != subaccounts.end()) return;

        subaccounts.emplace(parent, [&]( auto& item ) {
            item.account = account;
        });
    }
}