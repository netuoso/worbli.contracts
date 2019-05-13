#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/time.hpp>
#include "worblitimelock.hpp"


using namespace eosio;
using std::string;

class [[eosio::contract]] worblitimelock : public eosio::contract {
public:
  worblitimelock( name self, name code, datastream<const char*> ds )
  :contract(self, code, ds),
    _conditions(self, self.value),
    _recipients(self, self.value), 
    _variables(self, self.value)
  {  }

  // Each user meets certain criteria and gets a specified percentage of tokens released.
  // Description is used in payout memo
  struct [[eosio::table("conditions")]] condition {
    name            cond; 
    time_point_sec  release_time; // number of days from base_time to release tokens
    uint64_t        tpercent; // in thousands of percent (1000 = 1%)
    string          description;
    uint64_t primary_key()const { return cond.value; }

    EOSLIB_SERIALIZE( condition, (cond)(release_time)(tpercent)(description) )
  };

  typedef eosio::multi_index<name("conditions"), condition> conditions;

  
  // Set up a condition name and corresoinding percentage of holdings to release to the recipient.
  // tpercent in thousands of percent (1000 = 1%)
  [[eosio::action]]
  void setcondition(name cond, uint64_t tpercent, const string description, time_point_sec releasetime)
  {
    require_auth(WBI_TIMELOCK_ADMIN);    
    auto conditr = _conditions.find(cond.value);
    if( conditr != _conditions.end() ) {
      _conditions.modify( *conditr, _self, [&]( auto& item ) {
          item.cond = cond;
          item.release_time = releasetime;
          item.tpercent = tpercent;
          item.description = description;
        });
    }
    else {
      _conditions.emplace(_self, [&]( auto& item ) {
          item.cond = cond;
          item.release_time = releasetime;
          item.tpercent = tpercent;
          item.description = description;
        });
    }
  }



  struct [[eosio::table("recipients")]] recipient {
    name           owner;
    asset          total_tokens;   // amount of WBI that the user may theoretically receive
    asset          locked_tokens;  // amount of WBI that the user has not yet received
    std::vector<name>   conditions;     // conditions already satisfied
    uint64_t primary_key()const { return owner.value; }
  };

  typedef eosio::multi_index<name("recipients"), recipient> recipients;


  struct transfer
  {
    name         from;
    name         to;
    asset        quantity;
    string       memo;
  };

    // This adds a new WBI token recipient and their total amount of WBI.
  // If the recipient exists already, their amount of tokens adds up, and if the
  // user has met certain conditions, corresponding amount of WBI is transferred
  // to the user.
  // The method verifies that our liability is within our total fund.
  [[eosio::action]]
  void addrcpnt (name owner, asset amount)
  {
    require_auth(WBI_TIMELOCK_ADMIN);
    check( is_account( owner ), "owner account does not exist");
    check(amount.symbol == WBI_SYMBOL, "invalid symbol");
    check(amount.amount >= 0, "negative amount");
        
    _add_liabilities(amount);
    
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr == _recipients.end(), "recipient already exists, please use updatercpnt");

    _recipients.emplace(_self, [&]( auto& item ) {
        item.owner = owner;
        item.total_tokens = amount;
        item.locked_tokens = amount;
    });

  }
  
  
  // This adds a new WBI token recipient and their total amount of WBI.
  // If the recipient exists already, their amount of tokens adds up, and if the
  // user has met certain conditions, corresponding amount of WBI is transferred
  // to the user.
  // The method verifies that our liability is within our total fund.
  [[eosio::action]]
  void addrcpnt1 (name owner, asset total, asset locked, std::vector<name> &met_conditions)
  {
    require_auth(WBI_TIMELOCK_ADMIN);
    check( is_account( owner ), "owner account does not exist");
    check(total.symbol == WBI_SYMBOL, "invalid symbol");
    check(locked.symbol == WBI_SYMBOL, "invalid symbol");

    check(total.amount >= 0, "total is negative amount");
    check(locked.amount >= 0, "locked is negative amount");

    check(locked.amount <= total.amount, "locked must be less than total");
        
    _add_liabilities(locked);
    
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr == _recipients.end(), "recipient already exists, please use updatercpnt");

    _recipients.emplace(_self, [&]( auto& item ) {
        item.owner = owner;
        item.total_tokens = total;
        item.locked_tokens = locked;
    });

    rcptitr = _recipients.find(owner.value);
    check(rcptitr != _recipients.end(), "recipient not found, logic error");

    for( const auto& cond: met_conditions ) {
      auto conditr = _conditions.find(cond.value);
      check(conditr != _conditions.end(), "cannot find condition");
      _recipients.modify( *rcptitr, _self, [&]( auto& item ) {
        item.conditions.emplace_back(cond);
      });            
    }  

  }

  // This adds a new WBI token recipient and their total amount of WBI.
  // If the recipient exists already, their amount of tokens adds up, and if the
  // user has met certain conditions, corresponding amount of WBI is transferred
  // to the user.
  // The method verifies that our liability is within our total fund.
  [[eosio::action]]
  void updatercpnt (name owner, asset amount)
  {
    require_auth(WBI_TIMELOCK_ADMIN);
    check( is_account( owner ), "owner account does not exist");
    check(amount.symbol == WBI_SYMBOL, "invalid symbol");
    check(amount.amount >= 0, "negative amount");

    _add_liabilities(amount);
    
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr != _recipients.end(), "recipient does not exist, please use addrcpnt");
    check(amount.amount + rcptitr->locked_tokens.amount >= 0 , "this would result in over payment");
    _recipients.modify( *rcptitr, _self, [&]( auto& item ) {
        item.total_tokens += amount;
        item.locked_tokens += amount;
    });

    for(auto const& cond: rcptitr->conditions) {
      _release_tokens(owner, cond, amount);
    }  
 
  }

  [[eosio::action]]
  void claim (name owner)
  {
    require_auth(owner);
    check( is_account( owner ), "owner account does not exist");
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr != _recipients.end(), "cannot find the owner in the database");

    for (auto itr = _conditions.begin(); itr != _conditions.end(); itr++) {
      auto cnditr = std::find(rcptitr->conditions.begin(), rcptitr->conditions.end(), (*itr).cond);
      
      if( cnditr != rcptitr->conditions.end() ) continue;        
      if( current_time_point() < time_point((*itr).release_time) ) continue;

      _recipients.modify( *rcptitr, _self, [&]( auto& item ) {
        item.conditions.emplace_back((*itr).cond);
      });

      _release_tokens(owner, (*itr).cond, rcptitr->total_tokens);
      print("release tokens: ", (*itr).release_time.sec_since_epoch(), "\n");
 
    }
  }  
  
  /**
   * Skip condition (mark as satisfied but do not transfer)
   * This is for founders who already recieved some tranches
  **/
  [[eosio::action]]
  void skip (name owner, name cond)
  {
    require_auth(WBI_TIMELOCK_ADMIN);
    check( is_account( owner ), "owner account does not exist");
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr != _recipients.end(), "cannot find the owner in the database");
    auto conditr = _conditions.find(cond.value);
    check(conditr != _conditions.end(), "cannot find this condition name");

    // the same condition cannot be achieved twice
    auto cnditr = std::find(rcptitr->conditions.begin(), rcptitr->conditions.end(), cond);
    check(cnditr == rcptitr->conditions.end(), "This condition is already met for the account");

    _recipients.modify( *rcptitr, _self, [&]( auto& item ) {
        item.conditions.emplace_back(cond);
      });

    _release_tokens(owner, cond, rcptitr->total_tokens);
  }

  // Release a certain percentage of holdings to the recipient because a certain condition is met.
  [[eosio::action]]
  void release (name owner, name cond)
  {
    require_auth(WBI_TIMELOCK_ADMIN);
    check( is_account( owner ), "owner account does not exist");
    auto rcptitr = _recipients.find(owner.value);
    check(rcptitr != _recipients.end(), "cannot find the owner in the database");
    auto conditr = _conditions.find(cond.value);
    check(conditr != _conditions.end(), "cannot find this condition name");

    // the same condition cannot be achieved twice
    auto cnditr = std::find(rcptitr->conditions.begin(), rcptitr->conditions.end(), cond);
    check(cnditr == rcptitr->conditions.end(), "This condition is already met for the account");

    _recipients.modify( *rcptitr, _self, [&]( auto& item ) {
        item.conditions.emplace_back(cond);
      });

    _release_tokens(owner, cond, rcptitr->total_tokens);
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


  // eosio.token balance
  struct account {
    asset    balance;
    uint64_t primary_key()const { return balance.symbol.raw(); }
  };
  
  typedef eosio::multi_index<name("accounts"), account> accounts;

  // this works for negative amounts too
  void _add_liabilities(asset amount)
  {
    // retrieve our WBI balance
    accounts accounts_index(name("eosio.token"), _self.value);
    asset current_balance =
      accounts_index.get(WBI_SYMBOL.code().raw(), "no balance object found").balance;

    // make sure liabilities are affordable
    int64_t total_liablilities = _getvar_int(name("liabilities")) + amount.amount;
    check(total_liablilities >= 0, "Negative total liabilities");
    check(total_liablilities <= current_balance.amount, "insufficient funds on escrow account");

    _setvar_int(name("liabilities"), total_liablilities);
  }

   time_point current_time_point() {
      const static time_point ct{ microseconds{ static_cast<int64_t>( current_time() ) } };
      return ct;
   }
  
  void _release_tokens(name owner, name cond, asset base, bool trans = true)
  {
    auto& rcpt = _recipients.get(owner.value);
    auto& cnd = _conditions.get(cond.value);
    
    // all tokens might already be released, but the user meets a new condition
    if( rcpt.locked_tokens.amount > 0 ) { 
      // Calculate the amount to transfer
      asset to_release(0, WBI_SYMBOL);

      // if a new amount is added, we release only percentage of added WBI
      to_release.amount = base.amount * cnd.tpercent / 100000;
      
      if( to_release > rcpt.locked_tokens ) {
        to_release = rcpt.locked_tokens;
      }
      
      _add_liabilities(-to_release);  // decrease liabilities and unlock tokens
      _recipients.modify( rcpt, _self, [&]( auto& item ) {
          item.locked_tokens -= to_release;
        });

      // send released tokens to the owner
      if (!trans) return;

      action
        {
          permission_level{_self, name("payout")},
            name("eosio.token"),
              name("transfer"),
              transfer  {
              .from=_self, .to=rcpt.owner,
                .quantity=to_release, .memo=cnd.description
                }
        }.send();
    }
  }
  
  conditions _conditions;
  recipients _recipients;
  variables _variables;
};

EOSIO_DISPATCH( worblitimelock, (setcondition)(updatercpnt)(addrcpnt)(claim) )
