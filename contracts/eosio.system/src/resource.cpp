#include <eosio.system/eosio.system.hpp>
#include <eosio.token/eosio.token.hpp>


namespace eosiosystem {

    ACTION system_contract::settotal(name source, uint64_t total_cpu_us, uint64_t total_net_words, time_point_sec timestamp)
    {
        require_auth(source);
        check(is_source(source) == true, "not authorized to execute this action");
        check(!_resource_config_state.locked, "prior collection period is still open");

        print(" :: total_cpu_us: ");
        print(std::to_string(total_cpu_us));

        system_usage_table u_t(get_self(), get_self().value);
        auto itr = u_t.end();
        itr--;

        time_point_sec next = time_point_sec(86400 + itr->timestamp.sec_since_epoch());
        
        /**
        * timestamp must be 24 hours after last history entry
        * time stamp cannot be greater than previous day
        **/
        // 
        check(timestamp == next, "invalid timestamp");
        check(next <=  time_point_sec(current_time_point().sec_since_epoch() - 86400), "cannot settotal for future date");

        // Initial Inflation
        float VT = 0.0185; // Initial Inflation Constant for Value Transfer
        float MP = 0.2947; // TO-DO: move these 2 constants to the config talbe for easier adjusting?

        uint64_t draglimit = _resource_config_state.emadraglimit;
        uint64_t day_count = itr->daycount + 1;

        float previousAverageCPU = itr->ma_cpu;
        float previousAverageNET = itr->ma_net;

        /**
        * Turn utility readings into ratios
        **/
        uint64_t system_max_cpu = static_cast<uint64_t>(_gstate.max_block_cpu_usage) * 2 * 60 * 60 * 24;
        float usage_cpu = static_cast<float>(total_cpu_us) / system_max_cpu;

        uint64_t system_max_net = static_cast<uint64_t>(_gstate.max_block_net_usage) * 2 * 60 * 60 * 24;
        float usage_net = static_cast<float>(total_net_words * 8) / system_max_net;

        float net_percent_total = usage_net / (usage_net + usage_cpu);
        float cpu_percent_total = usage_cpu / (usage_net + usage_cpu);

         print(" :: system_max_net: ");
         print(std::to_string(system_max_net));

         print(" :: system_max_cpu: ");
         print(std::to_string(system_max_cpu));

         print(" :: usage_cpu: ");
         print(std::to_string(usage_cpu));

        float ma_cpu_total = 0.0;
        float ma_net_total = 0.0;

        itr = u_t.end();
        for (int i = 1; i < draglimit; i++)
        {
            itr--;
            ma_cpu_total += itr->use_cpu;
            ma_net_total += itr->use_net;

            if (itr == u_t.begin())
            {
                break;
            }
        }

        // calculate period for moving averages during bootstrap period
        uint8_t period = day_count < draglimit ? day_count + 1 : draglimit;

        print(" :: period: ");
        print(std::to_string(period));

        float UTIL_CPU_MA = worbli::calcMA(ma_cpu_total, period, usage_cpu);
        float UTIL_NET_MA = worbli::calcMA(ma_net_total, period, usage_net);

        float UTIL_CPU_EMA;
        float UTIL_NET_EMA;

        uint64_t pk = u_t.available_primary_key();

        // use simple moving average until we reach draglimit samples
        if (pk >= draglimit)
        {
            UTIL_CPU_EMA = worbli::calcEMA(previousAverageCPU, draglimit, usage_cpu);
            UTIL_NET_EMA = worbli::calcEMA(previousAverageNET, draglimit, usage_net);
        }
        else
        {
            UTIL_CPU_EMA = UTIL_CPU_MA;
            UTIL_NET_EMA = UTIL_NET_MA;
        }
        float UTIL_TOTAL_EMA = UTIL_CPU_EMA + UTIL_NET_EMA;

        float inflation = (1 - UTIL_TOTAL_EMA) / (1 - UTIL_TOTAL_EMA - worbli::get_c(UTIL_TOTAL_EMA) * VT) - 1;

        float BP_U = MP * worbli::get_c(UTIL_TOTAL_EMA);
        float Upaygross = pow((1 + inflation), (1 - BP_U)) - 1;
        float Bppay = inflation - Upaygross;

        print(" :: UTIL_TOTAL_EMA: ");
        print(std::to_string(UTIL_TOTAL_EMA));

        print(" :: inflation: ");
        print(std::to_string(inflation));

        print(" :: BP_U: ");
        print(std::to_string(BP_U));

        print(" :: Upaygross: ");
        print(std::to_string(Upaygross));

        print(" :: Bppay: ");
        print(std::to_string(Bppay));

        const asset token_supply = eosio::token::get_supply(token_account, core_symbol().code());

        // Inflation waterfall
        float Min_Upaynet = inflation * UTIL_TOTAL_EMA;

        print(" :: Min_Upaynet: ");
        print(std::to_string(Min_Upaynet));

        float Waterfall_bp = inflation * (1 - UTIL_TOTAL_EMA);

        print(" :: Waterfall_bp: ");
        print(std::to_string(Waterfall_bp));

        float Bppay_final = fmin(Bppay, Waterfall_bp);

        print(" :: Bppay_final: ");
        print(std::to_string(Bppay_final));

        float Uppaynet = inflation - Bppay_final;

        print(" :: Uppaynet: ");
        print(std::to_string(Uppaynet));

        double Daily_i_U = pow(1 + inflation, static_cast<double>(1) / 365) - 1;

        print(" :: Daily_i_U: ");
        print(std::to_string(Daily_i_U));

        float utility_daily = (Uppaynet / inflation) * Daily_i_U;				                //allocate proportionally to Utility
        float bppay_daily = (Bppay_final / inflation) * Daily_i_U;				              //allocate proportionally to BPs

        float cpu_daily = cpu_percent_total * utility_daily;
        float net_daily = utility_daily - cpu_daily;


        // calculate inflation amount
        auto utility_tokens = static_cast<int64_t>( (cpu_daily * double(token_supply.amount)));
        auto bppay_tokens = static_cast<int64_t>( ((bppay_daily + cpu_daily) * double(token_supply.amount)));
        auto net_tokens = static_cast<int64_t>( (net_daily * double(token_supply.amount)));

        print(" :: utility_tokens: ");
        print(std::to_string(utility_tokens));

        u_t.emplace(get_self(), [&](auto &h) {
            h.id = pk;
            h.timestamp = timestamp;
            h.daycount = day_count;
            h.total_cpu_us = total_cpu_us;
            h.total_net_words = total_net_words;
            h.net_percent_total = net_percent_total;
            h.cpu_percent_total = cpu_percent_total;
            h.use_cpu = usage_cpu;
            h.use_net = usage_net;
            h.ma_cpu = UTIL_CPU_MA;
            h.ma_net = UTIL_NET_MA;
            h.ema_cpu = UTIL_CPU_EMA;
            h.ema_net = UTIL_NET_EMA;
            h.utility_daily = utility_daily;
            h.bppay_daily = bppay_daily;
            h.inflation = inflation;
            h.inflation_daily = Daily_i_U;
            h.utility_tokens = asset(utility_tokens, core_symbol() );
            h.bppay_tokens = asset(bppay_tokens, core_symbol() );
            h.net_tokens = asset(net_tokens, core_symbol() );
        });

        // open contract for user stats
        _resource_config_state.locked = true;

        // reset totals for user stats
        _resource_config_state.allocated_cpu = 0.0;
        _resource_config_state.allocated_net = 0.0;
        _resource_config_state.allocated_total = 0.0;
        _resource_config_state.utility_net_pay = asset( 0, core_symbol() );
        _resource_config_state.utility_cpu_pay = asset( 0, core_symbol() );

        debug_table d_t(get_self(), get_self().value);
        d_t.emplace(get_self(), [&](auto &d) {
            d.id = pk;
            d.timestamp = timestamp;
            d.total_cpu_us = total_cpu_us;
            d.total_net_words = total_net_words;
            d.usage_cpu = usage_cpu;
            d.usage_net = usage_net;
            d.net_percent_total = net_percent_total;
            d.cpu_percent_total = cpu_percent_total;
            d.ma_cpu = UTIL_CPU_MA;
            d.ma_net = UTIL_NET_MA;
            d.ema_cpu = UTIL_CPU_EMA;
            d.ema_net = UTIL_NET_EMA;
            d.inflation = inflation;
            d.inflation_daily = Daily_i_U;
            d.utility_daily = utility_daily;
            d.cpu_utility_daily = cpu_daily;
            d.net_utility_daily = net_daily;
            d.bppay_daily = bppay_daily;
        });

    }

    /**
    * Todo: close the system usage total and issue the daily inflation.
    **/
    ACTION system_contract::commitusage(name source, time_point_sec timestamp) {
        require_auth(source);
        check(is_source(source) == true, "not authorized to execute this action");
        check(_resource_config_state.locked, "distribution is closed");

        system_usage_table u_t(get_self(), get_self().value);
        auto itr_u = u_t.end();
        itr_u--;

        check(itr_u->timestamp == timestamp, "timestamp is not correct, collecting stats for: " + std::to_string(timestamp.sec_since_epoch()));

        {
            token::issue_action issue_act{token_account, {{get_self(), active_permission}}};
            issue_act.send(get_self(), itr_u->bppay_tokens + itr_u->utility_tokens, "issue daily inflation");
         }
         {
            token::transfer_action transfer_act{token_account, {{get_self(), active_permission}}};
            transfer_act.send(get_self(), ppay_account, itr_u->bppay_tokens, "producer daily");
            transfer_act.send(get_self(), usage_account, itr_u->utility_tokens, "usage daily");
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

         uint64_t earned_pay = uint64_t(itr_u->bppay_tokens.amount / active_producers.size());
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

        _resource_config_state.locked = false; 
        _wgstate.last_inflation_print = timestamp;

    }

    ACTION system_contract::adddistrib(name source, name account, uint64_t user_cpu_us, uint64_t user_net_words, time_point_sec timestamp)
    {  
        require_auth(source);
        check(is_account(account), account.to_string() + " is not an account");
        check(is_source(source) == true, "not authorized to execute this action");;
        check(_resource_config_state.locked, "the collection period has been closed");

        system_usage_table u_t(get_self(), get_self().value);
        auto itr_u = u_t.end();
        itr_u--;

        check(itr_u->timestamp == timestamp, "timestamp is not correct, collecting stats for: " + std::to_string(timestamp.sec_since_epoch()));

        float net_percentage = static_cast<float>( user_net_words ) / itr_u->total_net_words;
        float cpu_percentage = static_cast<float>( user_cpu_us ) / itr_u->total_cpu_us;

        check(_resource_config_state.allocated_cpu + user_cpu_us <= itr_u->total_cpu_us, "cpu allocation greater than 100%" );
        check(_resource_config_state.allocated_net + user_net_words <= itr_u->total_net_words, "net allocation greater than 100%" );
        
        // TODO: look into making allocated_total an integer
        check(_resource_config_state.allocated_total + net_percentage + cpu_percentage <= 100, "total resource allocation greater than 100%" );

        _resource_config_state.allocated_total += net_percentage;
        _resource_config_state.allocated_cpu += user_cpu_us;
        _resource_config_state.allocated_net += user_net_words;
        
        auto utility_net_pay = net_percentage * itr_u->net_percent_total * itr_u->utility_tokens.amount;
        _resource_config_state.utility_net_pay += asset( utility_net_pay, core_symbol() );

        // utility_daily is a percentage
        auto add_claim = cpu_percentage * itr_u->cpu_percent_total * itr_u->utility_tokens.amount;
        asset payout = asset( add_claim, core_symbol() );
        _resource_config_state.utility_cpu_pay += payout;

        account_pay_table a_t(get_self(), get_self().value);
        auto itr_d = a_t.find(account.value);
        if( itr_d == a_t.end() ) {
            a_t.emplace(get_self(), [&](auto &d) {
                d.account = account;
                d.timestamp = timestamp;
                d.payout = payout;
            });
        } else {
            check(itr_d->timestamp != timestamp, "duplicate distribution" );
            a_t.modify(itr_d, same_payer, [&](auto &d) {
                d.timestamp = timestamp;
                d.payout += payout;
            });
        }
    }

    ACTION system_contract::claimdistrib(name account)
    {

        require_auth(account);
        check(!_resource_config_state.locked, "cannot claim while inflation calculation is running");

        account_pay_table a_t(get_self(), get_self().value);
        auto itr = a_t.find(account.value);
        check(itr != a_t.end(), "account not found");
        check(itr->payout != asset( 0, core_symbol() ), "zero balance to claim");

        // transfer payout
        itr = a_t.erase(itr);

    }

    ACTION system_contract::updconfig(bool paused, uint32_t emadraglimit)
    {
        require_auth(get_self());
        check(emadraglimit > 0, "ema drag must be greater than 0");
        check(emadraglimit < 20, "ema drag must be less than 20");
        _resource_config_state.paused = paused;
        _resource_config_state.emadraglimit = emadraglimit;
    }

    ACTION system_contract::addupdsource(name account, uint8_t in_out)
    {
        require_auth(get_self());
        sourceauth_table s_t(get_self(), get_self().value);
        if (in_out == 0)
        {
            auto itr = s_t.find(account.value);
            check(itr != s_t.end(), "authorized source account not found during removal");
            itr = s_t.erase(itr);
        }
        else
        {
            auto itr2 = s_t.find(account.value);
            check(itr2 == s_t.end(), "authorized source account already exists in sourceauths table");
            s_t.emplace(get_self(), [&](auto &s) {
            s.account = account;
            });
        }
    }

    ACTION system_contract::initresource(time_point_sec start)
    {
        require_auth(get_self());
        system_usage_table u_t(get_self(), get_self().value);

        check(u_t.begin() == u_t.end(), "init already called");

        uint64_t pk = u_t.available_primary_key();
        u_t.emplace(get_self(), [&](auto &u) {
            u.id = pk;
            u.timestamp = start;
            u.use_cpu = 0;
            u.use_net = 0;
            u.daycount = 0;
            u.ma_cpu = 0;
            u.ma_net = 0;
            u.ema_cpu = 0;
            u.ema_net = 0;
            u.utility_daily = 0;
            u.bppay_daily = 0;
        });
    }

    bool system_contract::is_source(name source)
    {
        sourceauth_table s_t(get_self(), get_self().value);
        auto itr = s_t.find(source.value);
        if (itr == s_t.end())
        {
        return false;
        }
        else
        {
        return true;
        }
    }    

}