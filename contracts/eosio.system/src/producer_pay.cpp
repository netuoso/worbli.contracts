#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>

namespace eosiosystem
{

using eosio::current_time_point;
using eosio::microseconds;
using eosio::token;

void system_contract::onblock(ignore<block_header>)
{
   using namespace eosio;

   require_auth(get_self());

   const auto ct = current_time_point();

   block_timestamp timestamp;
   name producer;
   _ds >> timestamp >> producer;

   /** until activated no new rewards are paid */
   if (!_gstate.is_producer_schedule_active)
      return;

   if (_gstate.last_inflation_distribution == time_point()) /// start the presses
      _gstate.last_inflation_distribution = ct;

   /**
       * At startup the initial producer may not be one that is registered / elected
       * and therefore there may be no producer object for them.
       */

   auto prod = _producers.find(producer.value);
   if (prod != _producers.end())
   {
      _producers.modify(prod, same_payer, [&](auto &p) {
         p.produced_blocks++;
      });
   }

   /// only update block producers once every minute, block_timestamp is in half seconds
   if (timestamp.slot - _gstate.last_producer_schedule_update.slot > 120)
   {
      update_elected_producers(timestamp);
   }

   auto f_itr = _features.find("resource"_n.value);
   if (f_itr != _features.end() && f_itr->is_active == true)
   { // new resource model

      // grab locking and token supply metrics once a day
      if (ct.sec_since_epoch() - _wgstate.last_metric_read.sec_since_epoch() > seconds(seconds_per_day).to_seconds())
      {
         time_point_sec next = time_point_sec(86400 + _wgstate.last_metric_read.sec_since_epoch());
         const asset token_supply = eosio::token::get_supply(token_account, core_symbol().code());

         // TODO: store leasing table code and scope configuration in config table
         leasing_table l_t("leasing"_n, "leasing"_n.value);
         if (l_t.end() != l_t.begin()) {
            auto itr = l_t.begin();
            metrics_table m_t(get_self(), get_self().value);
            m_t.emplace(get_self(), [&](auto &item) {
               item.wbi_supply = token_supply;
               item.wbi_locked = itr->locked;
            });
         }
      }

      /**
          * Inflation issue requests are created by an oracle via the worbli.resource contract
          * As this is an offchain process it is possible for issue requests to be delayed for arbitrary durations
          * We need to enforce the following rules:
          * - last_inflation_print can only move forward in 24 hour increments
          * - new inflation request timestamp must be in the past
          *
          * We only need to check for a new issue request once last_inflation_print is > 24 hours in the past
          *
          * TODO: consider moving this to an action to eliminate "polling" the inflation table for new issue requests
          *
          * */
      if (ct.sec_since_epoch() - _wgstate.last_inflation_print.sec_since_epoch() > seconds(seconds_per_day).to_seconds())
      {
         inflation_table i_t(resource_account, resource_account.value);

         time_point_sec next = time_point_sec(86400 + _wgstate.last_inflation_print.sec_since_epoch());
         auto itr = i_t.find(next.sec_since_epoch());

         if (itr == i_t.end())
         {
            _wgstate.message = "issue request for timestamp " + std::to_string(next.sec_since_epoch()) +
                               " not found";
            return;
         }

         {
            token::issue_action issue_act{token_account, {{get_self(), active_permission}}};
            issue_act.send(get_self(), itr->amount, "issue tokens for daily inflation");
         }
         {
            token::transfer_action transfer_act{token_account, {{get_self(), active_permission}}};
            transfer_act.send(get_self(), resource_account, itr->amount, "daily inflation");
         }

         _wgstate.last_inflation_print = itr->timestamp;
      }
   }
   else
   { // legacy inflation model

      /// only distribute inflation once a day
      if (ct - _gstate.last_inflation_distribution > microseconds(useconds_per_day))
      {
         const asset token_supply = eosio::token::get_supply(token_account, core_symbol().code());
         const auto usecs_since_last_fill = ct - _gstate.last_inflation_distribution;
         auto new_tokens = static_cast<int64_t>((continuous_rate * double(token_supply.amount) * double(usecs_since_last_fill.count())) / double(useconds_per_year));

         /** Percentages are fixed point with a denominator of 1000 */
         const uint16_t base_producer_rate = 250;    // 25%
         const uint16_t incremental_usage_rate = 25; // 2.5%

         uint16_t actual_producer_rate = 0;

         if (_gstate.network_usage_level == 0)
            actual_producer_rate = base_producer_rate;
         else if ((_gstate.network_usage_level < 30))
            actual_producer_rate = base_producer_rate + incremental_usage_rate * _gstate.network_usage_level;
         else
            actual_producer_rate = 1000;

         auto max_to_producers = new_tokens / 6;
         auto to_producers = (max_to_producers * actual_producer_rate) / 1000;
         auto to_savings = max_to_producers + max_to_producers - to_producers;
         auto to_usage = new_tokens - (to_producers + to_savings);
         {
            token::issue_action issue_act{token_account, {{get_self(), active_permission}}};
            issue_act.send(get_self(), asset(new_tokens, core_symbol()), "issue tokens for producer pay and savings");
         }
         {
            token::transfer_action transfer_act{token_account, {{get_self(), active_permission}}};
            transfer_act.send(get_self(), saving_account, asset(to_savings, core_symbol()), "unallocated inflation");
            transfer_act.send(get_self(), ppay_account, asset(to_producers, core_symbol()), "fund producer account");
            transfer_act.send(get_self(), usage_account, asset(to_usage, core_symbol()), "fund usage account");
         }

         std::vector<name> active_producers;
         for (const auto &p : _producers)
         {
            if (p.active())
            {
               active_producers.emplace_back(p.owner);
            }
         }

         check(active_producers.size() == _gstate.last_producer_schedule_size, "active_producers must equal last_producer_schedule_size");

         uint64_t earned_pay = uint64_t(to_producers / active_producers.size());
         for (const auto &p : active_producers)
         {

            auto pay_itr = _producer_pay.find(p.value);

            if (pay_itr == _producer_pay.end())
            {
               pay_itr = _producer_pay.emplace(p, [&](auto &pay) {
                  pay.owner = p;
                  pay.earned_pay = earned_pay;
               });
            }
            else
            {
               _producer_pay.modify(pay_itr, same_payer, [&](auto &pay) {
                  pay.earned_pay += earned_pay;
               });
            }
         }

         _gstate.last_inflation_distribution = ct;
      }
   }
}

void system_contract::claimrewards(const name &owner)
{
   require_auth(owner);

   const auto &prod = _producers.get(owner.value);
   check(prod.active(), "producer does not have an active key");

   auto ct = current_time_point();

   //producer_pay_table  pay_tbl( _self, _self );
   auto pay = _producer_pay.find(owner.value);
   check(pay != _producer_pay.end(), "producer pay request not found");
   check(ct - prod.last_claim_time > microseconds(useconds_per_day), "already claimed rewards within past day");

   uint64_t earned_pay = pay->earned_pay;

   _producer_pay.erase(pay);

   _producers.modify(prod, same_payer, [&](auto &p) {
      p.last_claim_time = ct;
   });

   if (earned_pay > 0)
   {
      token::transfer_action transfer_act{token_account, {{ppay_account, active_permission}, {owner, active_permission}}};
      transfer_act.send(ppay_account, owner, asset(earned_pay, core_symbol()), "producer pay");
   }
}

} //namespace eosiosystem
