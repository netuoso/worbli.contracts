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


        print("fuckye\n");
        // insert new values
        for ( auto& info : existing ) {
            print("account: ", info.account);
            _account_info.emplace(_self, [&]( auto& item ) {
                item.account = info.account;
                item.parent = info.parent;
                item.kyc = info.kyc;
                item.max_subaccounts = -1;
            });

            print("name: ", info.account, " , children count: ", info.children.size());
            subaccount_table subaccounts(_self, info.account.value);
            for ( auto& sub : info.children ) {
                subaccounts.emplace(_self, [&]( auto& item ) {
                    item.account = sub;
                });

            }
        }
    }
}