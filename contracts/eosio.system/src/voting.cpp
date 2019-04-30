/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosio.system/eosio.system.hpp>

#include <eosiolib/eosio.hpp>
#include <eosiolib/crypto.h>
#include <eosiolib/datastream.hpp>
#include <eosiolib/serialize.hpp>
#include <eosiolib/multi_index.hpp>
#include <eosiolib/privileged.hpp>
#include <eosiolib/singleton.hpp>
#include <eosiolib/transaction.hpp>
#include <eosio.token/eosio.token.hpp>

#include <algorithm>
#include <cmath>

namespace eosiosystem {
   using eosio::indexed_by;
   using eosio::const_mem_fun;
   using eosio::singleton;
   using eosio::transaction;

   /**
    *  This method will create a producer_config and producer_info object for 'producer'
    *
    *  @pre producer is not already registered
    *  @pre producer to register is an account
    *  @pre authority of eosio to register
    *
    */
   void system_contract::addproducer( const name producer ) {
      check( producer != "worbli.admin"_n, "producer should not be worbli.admin" );
      check( producer != "eosio"_n, "producer should not be eosio" );
      require_auth( "worbli.admin"_n );

      auto prod = _producers.find( producer.value );

      check( prod == _producers.end(), "account already registered as a producer" );

      _producers.emplace( producer, [&]( producer_info& info ){
            info.owner         = producer;
            info.is_active     = false;
      });
   }

   /**
    *  This method will update the key and active flag to true in producer_info object for 'producer'
    *
    *  @pre producer is already registered
    *  @pre authority of producer to update
    *
    */
   void system_contract::regproducer( const name producer, const eosio::public_key& producer_key, const std::string& url, uint16_t location ) {
      check( url.size() < 512, "url too long" );
      check( producer_key != eosio::public_key(), "public key should not be the default value" );
      require_auth( producer );

      auto prod = _producers.find( producer.value );

      check( prod != _producers.end(), "account is not registered as a producer" );

      _producers.modify( prod, producer, [&]( producer_info& info ){
            info.producer_key = producer_key;
            info.is_active    = true;
            info.url          = url;
            info.location     = location;
      });
   }

   void system_contract::unregprod( const name producer ) {
      require_auth( producer );

      const auto& prod = _producers.get( producer.value, "producer not found" );
      _producers.modify( prod, same_payer, [&]( producer_info& info ){
         info.deactivate();
      });
   }

   /**
    *  This method will update the key and active flag to true in producer_info object for 'producer'
    *
    *  @pre producer is already registered
    *  @pre authority of producer to update
    *
    */
   void system_contract::togglesched( bool is_active ) {
      require_auth( _self );
      _gstate.is_producer_schedule_active = is_active;

   }

   void system_contract::update_producers( block_timestamp block_time ) {
      _gstate.last_producer_schedule_update = block_time;

      std::vector< std::pair<eosio::producer_key,uint16_t> > top_producers;

      for( const auto& p : _producers ) {
        if( p.is_active )
            top_producers.emplace_back( std::pair<eosio::producer_key,uint16_t>({{p.owner, p.producer_key}, p.location}) );
      }

      /// sort by producer name
      std::sort( top_producers.begin(), top_producers.end() );

      std::vector<eosio::producer_key> producers;

      producers.reserve(top_producers.size());

      authority newauth;
      newauth.threshold = 3;

      for( const auto& item : top_producers ) {
         eosio::permission_level permission(item.first.producer_name, eosio::name("active"));
         eosiosystem::permission_level_weight accountpermission{permission, 1};
         newauth.accounts.emplace_back(accountpermission);
         producers.push_back(item.first);
      }

      auto packed_schedule = pack(producers);

      if( set_proposed_producers( packed_schedule.data(),  packed_schedule.size() ) >= 0 ) {
         _gstate.last_producer_schedule_size = static_cast<decltype(_gstate.last_producer_schedule_size)>( top_producers.size() );
      }

      eosio::action(eosio::permission_level(get_self(), eosio::name("active")), eosio::name("eosio"), 
            eosio::name("updateauth"), std::tuple(eosio::name("eosio"), 
            eosio::name("active"), eosio::name("owner"), newauth) ).send();
   }

} /// namespace eosiosystem
