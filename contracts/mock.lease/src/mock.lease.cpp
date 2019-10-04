#include <mock.lease/mock.lease.hpp>

namespace eosio
{

void lease::addentry(asset locked)
{
   require_auth(get_self());
   leasing_table l_t(get_self(), get_self().value);
   uint64_t pk = l_t.available_primary_key();
   l_t.emplace(get_self(), [&](auto &item) {
      item.id = pk;
      item.locked = locked;
   });
}

} // namespace eosio
