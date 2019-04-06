#include <eosiolib/eosio.hpp>
#include <eosio.token/eosio.token.hpp>
using namespace eosio;

using registeredtoken::requirement;

class [[eosio::contract("worbli.registry")]] worbliregistry : public eosio::contract {
    public:
        using contract::contract;

        [[eosio::action]]
        void update(name account, requirement requirement);

        [[eosio::action]]
        void remove(name account, requirement requirement);

    private:

    typedef eosio::multi_index< "sendreqs"_n, requirement> sendreqs;
  
};