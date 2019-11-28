#include "worbli.system_tester.hpp"

BOOST_AUTO_TEST_SUITE(worbliresource_tests)

BOOST_FIXTURE_TEST_CASE( test_average_calculations, worbli_system_tester, * boost::unit_test::tolerance(1e+2) ) try {
   create_account_with_resources(N(producer), N(worbli.admin));
   BOOST_REQUIRE_EQUAL( success(), addprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();

   // start date is January 1, 2020 12:00:00 AM
   // 1577836869

   /**
    * start metric collection and inflation
    * collect metrics from Dec 29 2019 to current day (Jan 1 2020)
    **/
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00", "2019-09-18T23:59:59"));
   produce_block( fc::days(1) );

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-09-18T23:59:59" ) );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.005, 0.005, "2019-09-19T23:59:59") );
   
   // test averages with 1 period: total usage 1%
   auto history = get_system_usage(1);
   BOOST_REQUIRE_EQUAL( "0.00499999988824129", history["ma_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.00499999988824129", history["ma_net"] );
   BOOST_REQUIRE_EQUAL( "0.00499999988824129", history["ema_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.00499999988824129", history["ema_net"] );

   // test averages with 5 periods: total usage 10%
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 5 ) );
   produce_blocks( 6 );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-20T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-21T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-22T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-23T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-24T23:59:59") );

   history = get_system_usage(6);

   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ma_cpu"] );
   BOOST_REQUIRE_EQUAL( "0.05000000074505806", history["ma_net"] );

   //BOOST_REQUIRE_EQUAL( success(), setmetric( "2019-09-25T00:00:00.000", "10000.0000 WBI", "2500.0000 WBI" ) );
   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.05, 0.05, "2019-09-25T23:59:59") );

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
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();

   // start date is January 1, 2020 12:00:54 AM

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-12-30T23:59:59" ) );
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00", "2019-12-30T23:59:59"));

   produce_block( fc::days(1) );
   produce_blocks( 2 );

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.005, 0.005, "2019-12-31T23:59:59") );
   
   // future date should fail
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot settotal for future date" ),
      settotal("worbli.admin", 0.005, 0.005, "2020-01-01T23:59:59") );

   // past date should fail
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "invalid timestamp" ),
      settotal("worbli.admin", 0.005, 0.005, "2019-12-15T23:59:59") );

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( test_distributions, worbli_system_tester ) try {
   create_account_with_resources(N(producer), N(worbli.admin));
   BOOST_REQUIRE_EQUAL( success(), addprod( N(producer) ));
   BOOST_REQUIRE_EQUAL( success(), regprod( N(producer) ));
   activate();

   create_account_with_resources(N(user1), N(worbli.admin));
   create_account_with_resources(N(user2), N(worbli.admin));
   create_account_with_resources(N(user3), N(worbli.admin));

   BOOST_REQUIRE_EQUAL( success(), addupdsource( N(worbli.admin), 1 ) );
   BOOST_REQUIRE_EQUAL( success(), updconfig( false, 1 ) );

   BOOST_REQUIRE_EQUAL( success(), init_resource( "2019-09-18T23:59:59" ) );
   BOOST_REQUIRE_EQUAL( success(),  setwgstate("1970-01-01T00:00:00", "2019-09-18T23:59:59"));

   BOOST_REQUIRE_EQUAL( success(), settotal("worbli.admin", 0.005, 0.005, "2019-09-19T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), adddistrib("worbli.admin", "user1", 2.0, 0.005, "2019-09-19T23:59:59") );
   BOOST_REQUIRE_EQUAL( success(), adddistrib("worbli.admin", "user2", 2.5, 0.005, "2019-09-19T23:59:59") );
   //BOOST_REQUIRE_EQUAL( success(), adddistrib("worbli.admin", "user3", 99.0, 0.005, "2019-09-19T23:59:59") );
   std::cout << get_resource_config() << std::endl;

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
