#pragma once

#include <math.h>

namespace eosiosystem {

   using namespace eosio;

   struct [[eosio::table("resourceconf"), eosio::contract("eosio.system")]] resource_config_state
   {
      bool paused;
      uint32_t emadraglimit = 2;
      uint64_t allocated_cpu = 0;
      uint64_t allocated_net = 0;
      float allocated_total = 0.0;
      asset utility_net_pay;
      asset utility_cpu_pay;
      bool locked = false;
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] sourceauth
   {
      name account;
      uint64_t primary_key() const { return (account.value); }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] system_usage
   {
      uint64_t id;
      uint32_t daycount;
      uint64_t total_cpu_us;
      uint64_t total_net_words;
      float net_percent_total;
      float cpu_percent_total;
      float use_cpu;
      float use_net;
      float ma_cpu;
      float ma_net;
      float ema_cpu;
      float ema_net;
      float ema_util_total;
      float utility;
      float utility_daily;
      float bppay;
      float bppay_daily;
      float inflation;
      float inflation_daily;
      asset utility_tokens;
      asset bppay_tokens;
      asset net_tokens;
      time_point_sec timestamp;
      uint64_t primary_key() const { return (id); }
   };

   struct [[eosio::table, eosio::contract("eosio.system")]] account_pay
   {
      name account; //Worbli account consuming the resource
      asset payout; //WBI asset to pay for this period
      time_point_sec timestamp;
      uint64_t primary_key() const { return (account.value); }
   };

   typedef eosio::multi_index<"systemusage"_n, system_usage> system_usage_table;
   typedef eosio::singleton<"resourceconf"_n, resource_config_state> resource_config_singleton;
   typedef eosio::multi_index<"sourceauths"_n, sourceauth> sourceauth_table;
   typedef eosio::multi_index<"accountpay"_n, account_pay> account_pay_table;
}

namespace worbli {

  // calculate a moving average
  static float calcMA(float sum, uint8_t timeperiod, float newVal)
  {
    auto rslt = sum + newVal;
    return rslt / (timeperiod);
  }

  // calculate an exponential moving average
  static float calcEMA(float previousAverage, int timePeriod, float newVal)
  {
    auto mult = 2.0 / (timePeriod + 1.0);
    auto rslt = (newVal - previousAverage) * mult + previousAverage;
    return rslt;
  }

  static float get_c(float x)
  { // model C[x] = -x * ln(x) * exp(1)
    float p1 = -x;
    float p2 = log(float(x));
    float p3 = exp(1);
    return p1 * p2 * p3;
  }

  static float round(float var)
  {
    //array of chars to store number as a string.
    char str[40];
    // Print in string the value of var with 4 decimal points
    sprintf(str, "%.4f", var);
    // scan string value in var
    sscanf(str, "%f", &var);
    return var;
  }

}