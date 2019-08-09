#include <eosio.system/cleanup.hpp>

namespace eosiosystem {

    void system_contract::migrate() {

        require_auth("worbli.admin"_n);    

        std::vector<account_info_old> existing;
        account_info_table_old        account_info_old(_self, _self.value);

        // collect and empty exiting table
        auto ite = account_info_old.begin();
        while(ite != account_info_old.end()) {
            existing.emplace_back( *ite );
            ite = account_info_old.erase(ite);
        }

        // insert new values
        for ( auto& info : existing ) {
            _account_info.emplace(_self, [&]( auto& item ) {
                item.account = info.account;
                item.parent = info.parent;
                item.kyc = info.kyc;
                item.max_subaccounts = -1;
            });

            subaccount_table subaccounts(_self, info.account.value);
            uint8_t kyc = 0;
            if(info.account == "worbli.admin"_n) kyc = 1;
            for ( auto& sub : info.children ) {
                subaccounts.emplace(_self, [&]( auto& item ) {
                    item.account = sub;
                });

                auto child = _account_info.find(sub.value);
                if(child != _account_info.end()) continue;

                _account_info.emplace(_self, [&]( auto& item ) {
                    item.account = sub;
                    item.parent = info.account;
                    item.kyc = kyc;
                    item.max_subaccounts = -1;
                });

            }
        }
    }
}
