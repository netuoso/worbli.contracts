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


   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of eosio to register
    *
    */
   void system_contract::addproducer( const name producer ) {
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
   }

    void system_contract::addacctinfo(name account, name parent, int8_t kyc,
                                     int64_t max_subaccounts) {
        require_auth("worbli.admin"_n);    
        create_account_records(account, parent, kyc, max_subaccounts);
    }

    void system_contract::updacctinfo(name account, uint8_t kyc, 
                                     int64_t max_subaccounts) {
        require_auth("worbli.admin"_n);    
        auto itr = _account_info.find(account.value);
        check( itr != _account_info.end(), "account_info record not found" );

        _account_info.modify( *itr, same_payer, [&]( auto& item ) {
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });
    }

    // TODO: validate no other account claiming subaccount
    void system_contract::updparent(name account, name parent, name new_parent) {
        require_auth("worbli.admin"_n);

        auto act_itr = _account_info.find(account.value);
        check( act_itr != _account_info.end(), "account_info record not found" );
        _account_info.modify( *act_itr, new_parent, [&]( auto& item ) {
            item.parent = new_parent;
        });

        subaccount_table subaccounts_old(_self, parent.value);
        auto sub_itr = subaccounts_old.find(account.value);
        check( sub_itr != subaccounts_old.end(), "subaccount record not found" );
        subaccounts_old.erase(sub_itr);

        subaccount_table subaccounts_new(_self, new_parent.value);
        subaccounts_new.emplace(_self, [&]( auto& item ) {
            item.account = account;
        });
    }

    void system_contract::setwparams(uint64_t max_subaccounts) {
      require_auth( _self );
      _wstate.max_subaccounts = max_subaccounts;
    }

    void system_contract::setfeature(name feature, bool is_active) {
       require_auth( _self );
       feature_table f_t(get_self(), get_self().value);
       auto itr = f_t.find(feature.value);
       if( itr == f_t.end() ) {
          f_t.emplace(get_self(), [&](auto &f) {
            f.feature = feature;
            f.is_active = is_active;
         });
       } else {
          f_t.modify( *itr, same_payer, [&]( auto& item ) {
            item.is_active = is_active;
        });
       }
    }

   // worbli additions
    void native::can_create_subaccount(name creator) {

        if(creator == "worbli.admin"_n || creator == _self) return;

        account_info_table accountinfo(_self, _self.value);
        auto itr = accountinfo.find(creator.value);

        check( itr->kyc > 0, "not permitted to create subaccounts" );

        worbli_params_singleton worbliparams(_self, _self.value);
        worbli_params wstate = worbliparams.exists() ? worbliparams.get() : worbli_params{0};

        subaccount_table subaccounts(_self, creator.value);
        auto sub_count = std::distance(subaccounts.cbegin(),subaccounts.cend());

        // if accountinfo.max_subaccount < 0 use wstate.max_subaccount
        int64_t max_subaccounts = itr->max_subaccounts < 0 ? wstate.max_subaccounts : itr->max_subaccounts;
        check( max_subaccounts > sub_count, "subaccount limit reached" );

    }

    void native::create_account_records(name account, name parent, int8_t kyc,
                                int64_t max_subaccounts) {

        account_info_table accountinfo(_self, _self.value);
        auto act_itr = accountinfo.find(account.value);

        check( act_itr == accountinfo.end(), "account_info record exists, please call updacctinfo" );

        subaccount_table subaccounts(_self, parent.value);

        accountinfo.emplace(parent, [&]( auto& item ) {
            item.account = account;
            item.parent = parent;
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });

        auto sub_itr = subaccounts.find(account.value);
        if(sub_itr != subaccounts.end()) return;

        subaccounts.emplace(parent, [&]( auto& item ) {
            item.account = account;
        });
    }
}