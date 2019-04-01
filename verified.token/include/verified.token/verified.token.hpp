#include <eosiolib/eosio.hpp>
#include <eosio.token/eosio.token.hpp>
using namespace eosio;

class [[eosio::contract]] verifiedtoken : public eosio::contract {
    public:
        using contract::contract;

        [[eosio::action]]
        void addregistry(name registry);

        [[eosio::action]]
        void rmvregistry(name registry);

    private:

    typedef eosio::multi_index< "sendreqs"_n, token::requirement> sendreqs;
    typedef eosio::multi_index< "receivereqs"_n, token::requirement> receivereqs;
    typedef eosio::multi_index< "registries"_n, name> registries;


};