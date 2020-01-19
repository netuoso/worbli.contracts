#include "worbli.system_tester.hpp"

BOOST_AUTO_TEST_SUITE(worbliresource_tests)

BOOST_FIXTURE_TEST_CASE( test_average_calculations, worbli_system_tester, * boost::unit_test::tolerance(1e+2) ) try {
   create_account_with_resources(N(producer), N(worbli.admin));
   BOOST_REQUIRE_EQUAL( success(), addprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), promoteprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();
   BOOST_REQUIRE_EQUAL( success(), activatefeature("resource"));
   
   // start date is January 1, 2020 12:00:00 AM
   // 1577836869

   /**
    * start metric collection and inflation
    * collect metrics from Dec 29 2019 to current day (Jan 1 2020)
    **/
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00"));
   produce_block( fc::days(1) );

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-09-18T23:59:59" ) );

   // test averages with 1 period: total usage 1% - 0.5% net, 0.5% cpu
   // Should default to 1%
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2019-09-19T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-19T23:59:59") );
   
   auto history = get_system_usage(1);
   BOOST_REQUIRE_EQUAL( "0.00999999977648258", history["ma_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.00999999977648258", history["ma_net"] );
   BOOST_REQUIRE_EQUAL( "0.00999999977648258", history["ema_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.00999999977648258", history["ema_net"] );

   // test averages with 5 periods: total usage 10%
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 5 ) );
   produce_blocks( 6 );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-20T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-20T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-21T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-21T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-22T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-22T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-23T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-23T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-24T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-24T23:59:59") );

   history = get_system_usage(6);

   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ma_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ma_net"] );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 1728000000, 1132462080, core_sym::from_string("0.0000"), "2019-09-25T23:59:59") );

   history = get_system_usage(7);
   
   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ema_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ema_net"] );

   // Sep 25 2019 12:00:00 AM
   uint64_t sec_since_epoch = 1569369600;
   
   auto inflation = get_inflation(sec_since_epoch);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_resource_oracle, worbli_system_tester ) try {
   create_account_with_resources(N(producer), N(worbli.admin));
   BOOST_REQUIRE_EQUAL( success(), addprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), promoteprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();
   BOOST_REQUIRE_EQUAL( success(), activatefeature("resource"));

   // start date is January 1, 2020 12:00:54 AM

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-12-30T23:59:59" ) );
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00"));

   produce_block( fc::days(1) );
   produce_blocks( 2 );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2019-12-31T23:59:59") );

   // previous collection period still open
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "prior collection period is still open" ),
      settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2020-01-01T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-12-31T23:59:59") );

   // future date should fail
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot settotal for future date" ),
      settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2020-01-01T23:59:59") );

   // past date should fail
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "invalid timestamp" ),
      settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2019-12-15T23:59:59") );

   produce_block( fc::days(1) );
   produce_blocks( 2 );

   BOOST_REQUIRE_EQUAL( success(), 
      settotal("worbli.admin", 172800000, 113246208, core_sym::from_string("0.0000"), "2020-01-01T23:59:59") );
   
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2020-01-01T23:59:59") );


   produce_block( fc::days(1) );
   produce_blocks( 2 );

   // test max usage
   BOOST_REQUIRE_EQUAL( success(), 
      settotal("worbli.admin", 34560000000, 22649241600, core_sym::from_string("0.0000"), "2020-01-02T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2020-01-02T23:59:59") );


   produce_block( fc::days(1) );
   produce_blocks( 2 );

   // test over max cpu usage
   BOOST_REQUIRE_EQUAL( success(), 
      settotal("worbli.admin", 34560000001, 22649241601, core_sym::from_string("0.0000"), "2020-01-03T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2020-01-03T23:59:59") );
   

   produce_block( fc::days(1) );
   produce_blocks( 2 );

   // test over max net usage
   BOOST_REQUIRE_EQUAL( success(), 
      settotal("worbli.admin", 34560000000, 22649241601, core_sym::from_string("0.0000"), "2020-01-04T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2020-01-04T23:59:59") );

   //std::cout << get_global_state() << std::endl;

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_distributions, worbli_system_tester ) try {
   create_account_with_resources(N(producer), N(worbli.admin));
   BOOST_REQUIRE_EQUAL( success(), addprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), promoteprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();
   BOOST_REQUIRE_EQUAL( success(), activatefeature("resource"));

   create_account_with_resources(N(user1), N(worbli.admin));
   create_account_with_resources(N(user2), N(worbli.admin));
   create_account_with_resources(N(user3), N(worbli.admin));

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-09-18T23:59:59" ) );
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00"));

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 172800000, 56623104, core_sym::from_string("0.0000"), "2019-09-19T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), adddistrib("worbli.admin", "user1", 800000, 28311552, "2019-09-19T23:59:59") );
   
   // should fail on duplicate
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "duplicate distribution" ),
      adddistrib("worbli.admin", "user1", 800000, 28311552, "2019-09-19T23:59:59") );
   
   BOOST_REQUIRE_EQUAL( success(), adddistrib("worbli.admin", "user2", 172000000, 28311552, "2019-09-19T23:59:59") );

   // this should fail as we are over subscribed
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "net allocation greater than 100%" ),
      adddistrib("worbli.admin", "user3", 0, 1, "2019-09-19T23:59:59") );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cpu allocation greater than 100%" ),
      adddistrib("worbli.admin", "user3", 1, 0, "2019-09-19T23:59:59") );

   BOOST_REQUIRE_EQUAL( success(), commitusage("worbli.admin", "2019-09-19T23:59:59") );

   auto user1_pay = get_accountpay(N(user1))["payout"].as<asset>();
   auto user2_pay = get_accountpay(N(user2))["payout"].as<asset>();

   BOOST_REQUIRE_EQUAL( success(), claimdistrib("user1") );
   BOOST_REQUIRE_EQUAL( success(), claimdistrib("user2") );

   BOOST_REQUIRE_EQUAL( user1_pay == get_balance("user1"), true );
   BOOST_REQUIRE_EQUAL( user2_pay == get_balance("user2"), true );

   // calculate manually and make sure numbers line up.

   std::cout << get_system_usage(1) << std::endl;

   // test overage in usage
   // make sure both models don't collide


} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
