#include <eosio/eosio.hpp>
#include <eosio.system/eosio.system.hpp>

namespace eosiosystem {

   void system_contract::voteproducer( const name& voter_name, const std::vector<name>& producers ) {
      require_auth( voter_name );
      check(producers.size() <= 15, "vote limited to 15 producers");
      update_votes( voter_name, producers, true );
   }

   void system_contract::update_votes( const name& voter_name, const std::vector<name>& producers, bool voting ) {

      check( producers.size() <= 15, "attempt to vote for too many producers" );

      auto voter = _voters.find( voter_name.value );
      check( voter != _voters.end(), "user must stake before they can vote" ); /// staking creates voter object

      for ( auto& p : producers ) {
          auto itr = _producers.find(p.value);
          check(itr != _producers.end(), ( "producer " + itr->owner.to_string() + " is not currently registered" ).data());
      }

       _voters.modify( voter, same_payer, [&]( auto& av ) {
         av.producers = producers;
      });
   }


}