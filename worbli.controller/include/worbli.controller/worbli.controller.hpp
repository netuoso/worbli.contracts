#include <eosiolib/eosio.hpp>
#include <eosio.token/registered.token.hpp>

using namespace eosio;       

using registeredtoken::requirement;
using registeredtoken::registry;


class [[eosio::contract("worbli.controller")]] worblicontroller : public eosio::contract {
    public:
      worblicontroller( name self, name code, datastream<const char*> ds )
      :contract(self, code, ds),
        _sendreqs(self, self.value),
        _registries(self, self.value)
      {  }

        using contract::contract; 

        [[eosio::action]]
        void addsendreq( name key, name value );

        [[eosio::action]]
        void delsendreq( name key, name value );

        [[eosio::action]]
        void addrcvreq( symbol_code sym, name key, name value );

        [[eosio::action]]
        void delrcvreq( symbol_code sym, name key, name value );

        [[eosio::action]]
        void addregistry(name registry);

        [[eosio::action]]
        void delregistry(name registry);

    private:

         struct [[eosio::table]] currency_stats {
            asset    supply;
            asset    max_supply;
            name     issuer;

            uint64_t primary_key()const { return supply.symbol.code().raw(); }
         };

        typedef eosio::multi_index< "stat"_n, currency_stats > stats;

        struct [[eosio::table("sendreqs")]] : requirement{};
        typedef eosio::multi_index< "sendreqs"_n, requirement> sendreqs;

        struct [[eosio::table("receivereqs")]] : requirement{};
        typedef eosio::multi_index< "receivereqs"_n, requirement> receivereqs;

        struct [[eosio::table("registries")]] : registry{};
        typedef eosio::multi_index< "registries"_n, registry> registries;

        sendreqs _sendreqs;
        registries _registries;
};