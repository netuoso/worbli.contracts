#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/time.hpp>
#include "playgroundtwo.hpp"


using namespace eosio;
using std::string;

class [[eosio::contract]] playgroundtwo : public eosio::contract {
public:
  playgroundtwo( name self, name code, datastream<const char*> ds )
  :contract(self, code, ds),
    _testdata_table(self, self.value),
    _variables(self, self.value)
  {  }

  // Each user meets certain criteria and gets a specified percentage of tokens released.
  // Description is used in payout memo
  struct [[eosio::table("testdata")]] testdata {
    name            name_field; 
    std::vector<uint16_t> vect_field;
    string          str_field;
    uint16_t        int16_field;
    std::vector<name> vect_field2;

    uint64_t primary_key()const { return name_field.value; }
    
    EOSLIB_SERIALIZE( testdata, (name_field)(vect_field)(str_field)(int16_field)(vect_field2) )
  };

  typedef eosio::multi_index<name("testdata"), testdata> testdata_table;

  
  // Set up a condition name and corresoinding percentage of holdings to release to the recipient.
  // tpercent in thousands of percent (1000 = 1%)
  [[eosio::action]]
  void insrow(name name_field, string str_field, uint16_t int16_field, std::vector<uint16_t> vect_field, std::vector<name> vect_field2)
  {
    auto itr = _testdata_table.find(name_field.value);
    if( itr != _testdata_table.end() ) {
      _testdata_table.modify( *itr, _self, [&]( auto& item ) {
          item.name_field = name_field;
          item.str_field = str_field;
          item.int16_field = int16_field;
          item.vect_field = vect_field;
          item.vect_field2 = vect_field2;
        });
    }
    else {
      _testdata_table.emplace(_self, [&]( auto& item ) {
          item.name_field = name_field;
          item.str_field = str_field;
          item.int16_field = int16_field;
          item.vect_field = vect_field;
          item.vect_field2 = vect_field2;
        });
    }
  }  

  
  [[eosio::action]]
  void delrow(name name_field)
  {
    auto itr = _testdata_table.find(name_field.value);
    if( itr != _testdata_table.end() ) {
      _testdata_table.erase( itr );
    } 
  } 
  
private:

  struct [[eosio::table("variables")]] variable {
    name      key;
    int64_t   val_int;    
    uint64_t primary_key()const { return key.value; }
  };

  typedef eosio::multi_index<name("variables"), variable> variables;

  int64_t _getvar_int(name key)
  {
    auto varitr = _variables.find(key.value);
    if( varitr != _variables.end() ) {
      return varitr->val_int;
    }
    return 0;
  }

  void _setvar_int(name key, int64_t val)
  {
    auto varitr = _variables.find(key.value);
    if( varitr != _variables.end() ) {
      _variables.modify( *varitr, _self, [&]( auto& item ) {
          item.val_int = val;
        });
    }
    else {
      _variables.emplace(_self, [&]( auto& item ) {
          item.key = key;
          item.val_int = val;
        });
    }
  } 

  variables _variables;
  testdata_table _testdata_table;
};





EOSIO_DISPATCH( playgroundtwo, (insrow)(delrow) )
