#include <eosiolib/eosio.hpp>


using namespace eosio;
using std::string;
using eosio::const_mem_fun;

CONTRACT provider : public  contract
{
   using contract::contract;

public:

    TABLE registry
    {
        name            account;
        uint16_t        weight;
        string          key;
        std::vector<string>  value;

        uint64_t    primary_key() const { return account.value;}
    };

    TABLE registry1
    {
        uint64_t        pkey;
        name            account;
        uint16_t        weight;
        string          key;
        std::vector<string>  value;

        uint64_t    primary_key() const { return pkey;}
    };
    
    typedef eosio::multi_index<"registry"_n, registry > registry_table;
    typedef eosio::multi_index<"registry1"_n, registry1 > registry1_table;


    ACTION add( name registry, name account, uint16_t weight, string key, std::vector<string> value);
    ACTION add1( name registry, name account, uint16_t weight, string key, std::vector<string> value);
    ACTION update( name registry, name account, uint16_t weight, string key, std::vector<string> value);
    ACTION remove( name registry, name account, uint16_t weight, string key, std::vector<string> value);

};