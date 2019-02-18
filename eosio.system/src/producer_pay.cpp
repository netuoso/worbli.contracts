#include <eosio.system/eosio.system.hpp>

#include <eosio.token/eosio.token.hpp>

namespace eosiosystem {

   const int64_t  min_activated_stake   = 150'000'000'0000;
   const double   continuous_rate       = 0.05827323; // 6% annual rate
   const uint32_t blocks_per_year       = 52*7*24*2*3600;   // half seconds per year
   const uint32_t seconds_per_year      = 52*7*24*3600;
   const uint32_t blocks_per_day        = 2 * 24 * 3600;
   const uint32_t blocks_per_hour       = 2 * 3600;
   const uint64_t useconds_per_day      = 24 * 3600 * uint64_t(1000000);
   const uint64_t useconds_per_year     = seconds_per_year*1000000ll;
   const uint64_t useconds_per_min      = 60 * uint64_t(1000000);


   void system_contract::onblock( ignore<block_header> ) {
      using namespace eosio;

      require_auth(_self);

      const auto ct = current_time_point();

      block_timestamp timestamp;
      name producer;
      _ds >> timestamp >> producer;

      // _gstate2.last_block_num is not used anywhere in the system contract code anymore.
      // Although this field is deprecated, we will continue updating it for now until the last_block_num field
      // is eventually completely removed, at which point this line can be removed.
      _gstate2.last_block_num = timestamp;

      /** until activated no new rewards are paid */
      if( !_gstate.is_producer_schedule_active )
         return;

      if( _gstate.last_inflation_distribution == time_point() ) /// start the presses
         _gstate.last_inflation_distribution = ct;    

      /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */

      auto prod = _producers.find( producer.value );
      if ( prod != _producers.end() ) {
         _producers.modify( prod, same_payer, [&](auto& p ) {
               p.produced_blocks++;
         });
      }

      /// only update block producers once every minute, block_timestamp is in half seconds
      if( timestamp.slot - _gstate.last_producer_schedule_update.slot > 120 ) {
         update_producers( timestamp );
      }


      /// only distribute inflation once a day
      if( ct - _gstate.last_inflation_distribution > microseconds(useconds_per_day) ) {
         const asset token_supply   = eosio::token::get_supply(token_account, core_symbol().code() );
         const auto usecs_since_last_fill = ct - _gstate.last_inflation_distribution;
         auto new_tokens = static_cast<int64_t>( (continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill.count())) / double(useconds_per_year) );

      /** Percentages are fixed point with a denominator of 1000 */
         const uint16_t base_producer_rate = 250; // 25%
         const uint16_t incremental_usage_rate = 25; // 2.5%

         uint16_t actual_producer_rate = 0;   

         if(_gstate.network_usage_level == 0)
            actual_producer_rate = base_producer_rate;
         else if((_gstate.network_usage_level < 30))
            actual_producer_rate = base_producer_rate + incremental_usage_rate * _gstate.network_usage_level;
         else
            actual_producer_rate = 1000;

         auto max_to_producers   = new_tokens / 6;
         auto to_producers       = (max_to_producers * actual_producer_rate) / 1000;
         auto to_savings         = max_to_producers + max_to_producers - to_producers;
         auto to_usage           = new_tokens - (to_producers + to_savings);

         INLINE_ACTION_SENDER(eosio::token, issue)(
            token_account, { {_self, active_permission} },
            { _self, asset(new_tokens, core_symbol()), std::string("issue tokens for producer pay and savings and usage") }
         );

         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {_self, active_permission} },
            { _self, saving_account, asset(to_savings, core_symbol()), "unallocated inflation" }
         );

         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {_self, active_permission} },
            { _self, ppay_account, asset(to_producers, core_symbol()), "fund producer account" }
         );

         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {_self, active_permission} },
            { _self, usage_account, asset(to_usage, core_symbol()), "fund usage account" }
         );

        std::vector< name > active_producers;
        for( const auto& p : _producers ) {
            if( p.active() ) {
                active_producers.emplace_back( p.owner );
            }                
        }    

        eosio_assert( active_producers.size() == _gstate.last_producer_schedule_size, "active_producers must equal last_producer_schedule_size" );   

        uint64_t earned_pay = uint64_t(to_producers / active_producers.size());
        for( const auto& p : active_producers ) {

            auto pay_itr = _producer_pay.find( p.value );        

            if( pay_itr ==  _producer_pay.end() ) {
                pay_itr = _producer_pay.emplace( p, [&]( auto& pay ) {
                    pay.owner = p;
                    pay.earned_pay = earned_pay;
                });
            } else {
                _producer_pay.modify( pay_itr, same_payer, [&]( auto& pay ) {
                    pay.earned_pay += earned_pay;
                });
            }              
        }   

        _gstate.last_inflation_distribution = ct;

      }

   }

   using namespace eosio;
   void system_contract::claimrewards( const name owner ) {
      require_auth(owner);

      const auto& prod = _producers.get( owner.value );
      eosio_assert( prod.active(), "producer does not have an active key" );
                    
      auto ct = current_time_point();

      //producer_pay_table  pay_tbl( _self, _self );
      auto pay = _producer_pay.find( owner.value );
      eosio_assert( pay != _producer_pay.end(), "producer pay request not found" );
      eosio_assert( ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day" );

      uint64_t earned_pay = pay->earned_pay;

     _producer_pay.erase( pay );

      _producers.modify( prod, same_payer, [&](auto& p) {
          p.last_claim_time = ct;
      });

      if( earned_pay > 0 ) {
         INLINE_ACTION_SENDER(eosio::token, transfer)(
            token_account, { {ppay_account, active_permission}, {owner, active_permission} },
            { ppay_account, owner, asset(earned_pay, core_symbol()), std::string("producer block pay") }
         );
      }

   }

} //namespace eosiosystem
