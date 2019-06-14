#include <provider/provider.hpp>


ACTION provider::add(name registry, name account, uint16_t weight, string key, std::vector<string> value)
{
        registry_table  reg( _self, registry.value );
        auto entry_itr = reg.find( account.value );

        if( entry_itr ==  reg.end() ) {
            entry_itr = reg.emplace( _self, [&]( auto& entry ) {
                  entry.account = account;
                  entry.weight = weight;
                  entry.key = key;
                  entry.value = value;
                  });
        } else {
            reg.modify( entry_itr, _self, [&]( auto& entry ) {
                  entry.account = account;
                  entry.weight = weight;
                  entry.key = key;
                  entry.value = value;
                  });
        }

}


ACTION provider::add1(name registry, name account, uint16_t weight, string key, std::vector<string> value)
{

        registry1_table  reg1( _self, registry.value );
        auto entry1_itr = reg1.find( account.value );

        if( entry1_itr ==  reg1.end() ) {
            entry1_itr = reg1.emplace( _self, [&]( auto& entry ) {
                  entry.pkey = reg1.available_primary_key();
                  entry.account = account;
                  entry.weight = weight;
                  entry.key = key;
                  entry.value = value;
                  });
        } else {
            reg1.modify( entry1_itr, _self, [&]( auto& entry ) {
                  entry.account = account;
                  entry.weight = weight;
                  entry.key = key;
                  entry.value = value;
                  });
        }

}

ACTION provider::update( name registry, name account, uint16_t weight, string key, std::vector<string> value)
{}

ACTION provider::remove( name registry, name account, uint16_t weight, string key, std::vector<string> value)
{}


EOSIO_DISPATCH(provider,
  (add)(add1)(update)(remove)
);