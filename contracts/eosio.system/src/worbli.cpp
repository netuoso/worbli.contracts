namespace eosiosystem {

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
      _global.set( _gstate, _self );
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
        eosio_assert( itr != _account_info.end(), "account_info record not found" );

        _account_info.modify( *itr, same_payer, [&]( auto& item ) {
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });
    }

    // TODO: validate no other account claiming subaccount
    void system_contract::updparent(name account, name parent, name new_parent) {
        require_auth("worbli.admin"_n);

        auto act_itr = _account_info.find(account.value);
        eosio_assert( act_itr != _account_info.end(), "account_info record not found" );
        _account_info.modify( *act_itr, new_parent, [&]( auto& item ) {
            item.parent = new_parent;
        });

        subaccount_table subaccounts_old(_self, parent.value);
        auto sub_itr = subaccounts_old.find(account.value);
        eosio_assert( sub_itr != subaccounts_old.end(), "subaccount record not found" );
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

   // worbli additions
    void native::can_create_subaccount(name creator) {

        if(creator == "worbli.admin"_n || creator == _self) return;

        account_info_table accountinfo(_self, _self.value);
        auto itr = accountinfo.find(creator.value);

        eosio_assert( itr->kyc > 0, "not permitted to create subaccounts" );

        worbli_params_singleton worbliparams(_self, _self.value);
        worbli_params wstate = worbliparams.exists() ? worbliparams.get() : worbli_params{0};

        subaccount_table subaccounts(_self, creator.value);
        auto sub_count = std::distance(subaccounts.cbegin(),subaccounts.cend());

        // if accountinfo.max_subaccount < 0 use wstate.max_subaccount
        int64_t max_subaccounts = itr->max_subaccounts < 0 ? wstate.max_subaccounts : itr->max_subaccounts;
        eosio_assert( max_subaccounts > sub_count, "subaccount limit reached" );

    }

    void native::create_account_records(name account, name parent, int8_t kyc,
                                int64_t max_subaccounts) {

        account_info_table accountinfo(_self, _self.value);
        auto act_itr = accountinfo.find(account.value);

        eosio_assert( act_itr == accountinfo.end(), "account_info record exists, please call updacctinfo" );

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