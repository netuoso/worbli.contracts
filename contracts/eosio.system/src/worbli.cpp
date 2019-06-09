namespace eosiosystem {

    void system_contract::addaccount(name account, name parent, bool kyc, 
                                     std::vector<name> children, uint16_t max_children) {
        require_auth("worbli.admin"_n);    
        auto itr = _account_info.find(account.value);
        if( itr != _account_info.end() ) {
        _account_info.modify( *itr, _self, [&]( auto& item ) {
            item.account = account;
            item.parent = parent;
            item.kyc = kyc;
            item.children = children;
            item.max_children = max_children;
            });
        }
        else {
        _account_info.emplace(_self, [&]( auto& item ) {
            item.account = account;
            item.parent = parent;
            item.kyc = kyc;
            item.children = children;
            item.max_children = max_children;
            });
        }
    }

    void system_contract::updaccount(name account, bool kyc, uint16_t max_children) {
        require_auth("worbli.admin"_n);    
        auto itr = _account_info.find(account.value);
        eosio_assert( itr != _account_info.end(), "account_info record not found" ); 
        _account_info.modify( *itr, _self, [&]( auto& item ) {
            item.account = account;
            item.kyc = kyc;
            item.max_children = max_children;
        });
    }

}