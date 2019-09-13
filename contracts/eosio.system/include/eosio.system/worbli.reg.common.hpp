#include <eosio/eosio.hpp>
using namespace eosio;

#include <cmath>

namespace worbli_compliance
{

  using std::string;
  using std::vector;
  using std::find;

  struct [[ eosio::table, eosio::contract("worbli.reg") ]] attribute
  {
    name attribute_code;
    string description;
    uint64_t primary_key() const { return attribute_code.value; }

    EOSLIB_SERIALIZE(attribute, (attribute_code)(description))
  };

  typedef eosio::multi_index<name("attributes"), attribute> attributes;

  struct [[ eosio::table, eosio::contract("worbli.reg") ]] provider
  {
    name account;
    string description;
    vector<name> attributes;
    uint64_t primary_key() const { return account.value; }

    EOSLIB_SERIALIZE(provider, (account)(description)(attributes))
  };

  /**
   * TODO: How do we handle providers that exit the business (an active flag?)
  */

  typedef eosio::multi_index<name("providers"), provider> providers;

  struct [[ eosio::table, eosio::contract("provider") ]] account_attribute
  {
    name attribute_code;
    string value;
    uint64_t primary_key() const { return attribute_code.value; }

    EOSLIB_SERIALIZE(account_attribute, (attribute_code)(value))
  };

  typedef eosio::multi_index<name("registry"), account_attribute> registry;

    struct condition {
      name credential_code;
      vector<string> values;
    };

    inline vector<condition> validate(name provider, name account, vector<condition> conditions)
    {
      vector<condition> failed;

      for (condition condition : conditions)
      {
        registry registry_table(provider, account.value);
        auto itr = registry_table.find(condition.credential_code.value);

        if (itr == registry_table.end() ||
            find(condition.values.begin(), condition.values.end(), itr->value) == condition.values.end())
        {
          failed.push_back(condition);
        }
      }
      return failed;
    }

} // namespace worbli_compliance