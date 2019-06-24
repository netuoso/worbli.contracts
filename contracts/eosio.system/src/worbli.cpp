namespace eosiosystem {

    void system_contract::addacctinfo(name account, name parent, int8_t kyc,
                                     int64_t max_subaccounts) {
        require_auth("worbli.admin"_n);    

        auto act_itr = _account_info.find(account.value);
        eosio_assert( act_itr == _account_info.end(), "account_info record exists, please call updacctinfo" ); 

        subaccount_table subaccounts(_self, parent.value);

        _account_info.emplace(_self, [&]( auto& item ) {
            item.account = account;
            item.parent = parent;
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });

        subaccounts.emplace(_self, [&]( auto& item ) {
            item.account = account;
        });
    }

    void system_contract::updacctinfo(name account, uint8_t kyc, 
                                     int64_t max_subaccounts) {
        require_auth("worbli.admin"_n);    
        auto itr = _account_info.find(account.value);
        eosio_assert( itr != _account_info.end(), "account_info record not found" );

        _account_info.modify( *itr, _self, [&]( auto& item ) {
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });
    }

    // TODO: validate no other account claiming subaccount
    void system_contract::updparent(name account, name parent, name new_parent) {
        require_auth("worbli.admin"_n);

        auto act_itr = _account_info.find(account.value);
        eosio_assert( act_itr != _account_info.end(), "account_info record not found" );

        subaccount_table subaccounts_old(_self, act_itr->parent.value);
        auto sub_itr = subaccounts_old.find(account.value);
        eosio_assert( sub_itr != subaccounts_old.end(), "worbli data corrupt: subaccount record not found" );

        subaccounts_old.erase(sub_itr);

        subaccount_table subaccounts_new(_self, new_parent.value);
        subaccounts_new.emplace(_self, [&]( auto& item ) {
            item.account = account;
        });
    }

    void system_contract::setwparams(worbli_params& params) {
      require_auth( _self );
      (worbli_params&)(_wstate) = params;
      _worbliparams.set( _wstate, _self );
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

        accountinfo.emplace(_self, [&]( auto& item ) {
            item.account = account;
            item.parent = parent;
            item.kyc = kyc;
            item.max_subaccounts = max_subaccounts;
        });

        subaccounts.emplace(_self, [&]( auto& item ) {
            item.account = account;
        });
    }

}