#include <eosio.system/cleanup.hpp>

namespace eosiosystem
{
    void system_contract::migrate(name account)
    {
        require_auth("worbli.admin"_n);

        account_info_table_old account_info_old(_self, _self.value);
        auto old_itr = account_info_old.find(account.value);
        if (old_itr == account_info_old.end())
            return;

        account_info_old.erase(old_itr);
        // insert new values
        _account_info.emplace(_self, [&](auto &item) {
            item.account = old_itr->account;
            item.parent = old_itr->parent;
            item.max_subaccounts = old_itr->max_subaccounts;
        });
    }
} // namespace eosiosystem
