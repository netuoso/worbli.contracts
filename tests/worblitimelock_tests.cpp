#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include "eosio.system_tester.hpp"

#include "Runtime/Runtime.h"

#include <fc/variant_object.hpp>

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

class worblitimelock_tester : public tester {
public:

   worblitimelock_tester() {
      basic_setup();
      create_core_token();

      produce_blocks( 2 );

      create_accounts( { N(founder1), N(founder2), N(founder3), N(founders) } );
      produce_blocks( 2 );

      set_code( N(founders), contracts::worblitimelock_wasm() );
      set_abi( N(founders), contracts::worblitimelock_abi().data() );

      produce_blocks();

      const auto& accnt = control->db().get<account_object,by_name>( N(founders) );
      abi_def abi;
      BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
      abi_ser.set_abi(abi, abi_serializer_max_time);

      /**
      auto trace_auth = base_tester::push_action(config::system_account_name, updateauth::get_name(), N(founders), mvo()
                                            ("account", "founders")
                                            ("permission", "payout")
                                            ("parent", name(config::owner_name).to_string())
                                            ("auth",  authority(1, {}, {
                                                  permission_level_weight{{config::system_account_name, config::eosio_code_name}, 1}
                                               }
                                            ))
      );
      BOOST_REQUIRE_EQUAL(transaction_receipt::executed, trace_auth->receipt->status);
      **/

      set_authority(N(founders), 
                    "payout", 
                    authority(1,
                              vector<key_weight>{},
                              vector<permission_level_weight>{{{N(founders), N(eosio.code)}, 1}}
                     )
      );

      link_authority( N(founders), N(eosio.token),  N(payout), "transfer" );

   }

   void basic_setup() {
      produce_blocks( 2 );

      create_accounts({ N(eosio.token), N(worbli.admin)});

      produce_blocks( 100 );
      set_code( N(eosio.token), contracts::token_wasm());
      set_abi( N(eosio.token), contracts::token_abi().data() );
      {
         const auto& accnt = control->db().get<account_object,by_name>( N(eosio.token) );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         token_abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   void create_core_token( symbol core_symbol = symbol{CORE_SYM} ) {
      FC_ASSERT( core_symbol.precision() != 4, "create_core_token assumes precision of core token is 4" );
      create_currency( N(eosio.token), config::system_account_name, asset(100000000000000, core_symbol) );
      issue(config::system_account_name, asset(10000000000000, core_symbol) );
      BOOST_REQUIRE_EQUAL( asset(10000000000000, core_symbol), get_balance( "eosio", core_symbol ) );
   }

   asset get_balance( const account_name& act, symbol balance_symbol = symbol{CORE_SYM} ) {
      vector<char> data = get_row_by_account( N(eosio.token), act, N(accounts), balance_symbol.to_symbol_code().value );
      return data.empty() ? asset(0, balance_symbol) : token_abi_ser.binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
   }

   void issue( name to, const asset& amount, name manager = config::system_account_name ) {
      base_tester::push_action( N(eosio.token), N(issue), manager, mutable_variant_object()
                                ("to",      to )
                                ("quantity", amount )
                                ("memo", "")
                                );
   }

   void transfer( account_name from,
                  account_name to,
                  asset        quantity,
                  string       memo ) {
      base_tester::push_action( N(eosio.token), N(transfer), config::system_account_name, mutable_variant_object()
           ( "from", from)
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }


   void create_currency( name contract, name manager, asset maxsupply ) {
      auto act =  mutable_variant_object()
         ("issuer",       manager )
         ("maximum_supply", maxsupply );

      base_tester::push_action(contract, N(create), contract, act );
   }

   action_result push_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = abi_ser.get_action_type(name);

      action act;
      act.account = N(founders);
      act.name    = name;
      act.data    = abi_ser.variant_to_binary( action_type_name, data,abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), acc, N(accounts), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account", data, abi_serializer_max_time );
   }

   action_result add_recipient( account_name owner, asset amount ) {
      return push_action( N(worbli.admin), N(addrcpnt), mvo()
           ( "owner", owner )
           ( "amount", amount )
      );
   }

   action_result add_recipient1( account_name owner, asset total, 
                                asset locked, vector<account_name> met_conditions ) {
      return push_action( N(worbli.admin), N(addrcpnt1), mvo()
           ( "owner", owner )
           ( "total", total )
           ( "locked", locked )
           ( "met_conditions", met_conditions )
      );
   }

   action_result set_condition( account_name condition, uint64_t tpercent, 
                                string description, string release_time ) {
      return push_action( N(worbli.admin), N(setcondition), mvo()
           ( "cond", condition )
           ( "tpercent", tpercent )
           ( "description", description )
           ( "releasetime", release_time )
      );
   }

   action_result claim( account_name owner ) {
      return push_action( owner, N(claim), mvo()
           ( "owner", owner )
      );
   }

   fc::variant get_stats( const string& symbolname ) {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   asset get_token_supply() {
      return get_stats("4," CORE_SYM_NAME)["supply"].as<asset>();
   }

   fc::variant get_condition( name condition ) {
      vector<char> data = get_row_by_account( N(founders), N(founders), N(conditions), condition );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "condition", data, abi_serializer_max_time );
   }

   fc::variant get_recipient( name owner ) {
      vector<char> data = get_row_by_account( N(founders), N(founders), N(recipients), owner );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "recipient", data, abi_serializer_max_time );
   }

   uint32_t last_block_time() const {
      return time_point_sec( control->head_block_time() ).sec_since_epoch();
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
};

BOOST_AUTO_TEST_SUITE(worblitimelock_tests)

BOOST_FIXTURE_TEST_CASE( create_tests, worblitimelock_tester ) try {


   auto stats = get_stats("4," CORE_SYM_NAME);
   //std::cout << "supply: " << supply << std::endl;
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "1000000000.0000 TST")
      ("max_supply", "10000000000.0000 TST")
      ("issuer", "eosio")
   );

   // make sure contract is funded
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "no balance object found" ),
      add_recipient( N(founder1), core_sym::from_string("50000000.0000"), 
                               core_sym::from_string("50000000.0000"), 
                               vector<account_name>{}
      )
   );   

   transfer("eosio", "founders", core_sym::from_string("1.0000"), "escrow funding");

   // confirm sufficient funds
   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "insufficient funds on escrow account" ),
      add_recipient( N(founder1), core_sym::from_string("50000000.0000"), 
                               core_sym::from_string("50000000.0000"), 
                               vector<account_name>{}
      )
   );   

   transfer("eosio", "founders", core_sym::from_string("164050000.0000"), "escrow funding");
   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche1), 30000, "Tranche 1", "2018-11-15T00:00:00.000") );
   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche1), 30000, "Tranche 1", "2019-11-15T00:00:00.000") );
   
   BOOST_REQUIRE_EQUAL( success(), 
                        add_recipient( N(founder1), core_sym::from_string("50000000.0000"), 
                               core_sym::from_string("50000000.0000"), 
                               vector<account_name>{}
                        ) 
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "recipient already exists, please use updatercpnt" ), 
                        add_recipient( N(founder1), core_sym::from_string("50000000.0000"), 
                               core_sym::from_string("50000000.0000"), 
                               vector<account_name>{}
                        ) 
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot find condition" ), 
                        add_recipient( N(founder2), core_sym::from_string("50000000.0000"), 
                               core_sym::from_string("50000000.0000"), 
                               vector<account_name>{N(tranche1), N(tranche2)}
                        ) 
   );

   produce_blocks(1);

} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( standard_claim_tests, worblitimelock_tester ) try {

   auto stats = get_stats("4," CORE_SYM_NAME);
   //std::cout << "supply: " << supply << std::endl;
   REQUIRE_MATCHING_OBJECT( stats, mvo()
      ("supply", "1000000000.0000 TST")
      ("max_supply", "10000000000.0000 TST")
      ("issuer", "eosio")
   );

   transfer("eosio", "founders", core_sym::from_string("10000.0000"), "escrow funding");
   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche1), 30000, "Tranche 1", "2019-11-15T00:00:00.000") );
   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche1), 30000, "Tranche 1", "2020-05-15T00:00:00.000") );

   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche2), 15000, "Tranche 2", "2020-11-15T00:00:00.000") );
   BOOST_REQUIRE_EQUAL( success(), set_condition( N(tranche3), 75000, "Tranche 3", "2021-05-15T00:00:00.000") );

   // confirm condition has been updated
   auto cond = get_condition(N(tranche1));
   REQUIRE_MATCHING_OBJECT( cond, mvo()
      ("cond", "tranche1")
      ("release_time", "2020-05-15T00:00:00")
      ("tpercent", 30000)
      ("description", "Tranche 1")
   );


   cond = get_condition(N(tranche2));
   REQUIRE_MATCHING_OBJECT( cond, mvo()
      ("cond", "tranche2")
      ("release_time", "2020-11-15T00:00:00")
      ("tpercent", 15000)
      ("description", "Tranche 2")
   );


   cond = get_condition(N(tranche3));
   REQUIRE_MATCHING_OBJECT( cond, mvo()
      ("cond", "tranche3")
      ("release_time", "2021-05-15T00:00:00")
      ("tpercent", 75000)
      ("description", "Tranche 3")
   );
   
   BOOST_REQUIRE_EQUAL( success(), 
                        add_recipient( N(founder1), 
                                       core_sym::from_string("1000.0000"), 
                                       core_sym::from_string("1000.0000"), 
                                       vector<account_name>{}
                        ) 
   );

   BOOST_REQUIRE_EQUAL( success(), 
                        claim( N(founder1) ) 
   );

   BOOST_REQUIRE_EQUAL( success(), 
                        claim( N(founder1) ) 
   );

   BOOST_REQUIRE_EQUAL( wasm_assert_msg( "cannot find the owner in the database" ), 
                        claim( N(founder2) )  
   );

   // balance should be 0.0000 as release date has not been reached
   BOOST_REQUIRE_EQUAL( core_sym::from_string("0.0000"), 
                        get_balance(N(founder1))  
   );

   // produce for 5 1/2 months then claim again
   produce_block( fc::days(135) );

   BOOST_REQUIRE_EQUAL( success(), 
                        claim( N(founder1) ) 
   );

   // balance should be 300.0000 as tranche1 has been reached
   BOOST_REQUIRE_EQUAL( core_sym::from_string("300.0000"), 
                        get_balance(N(founder1))  
   );

   // produce 6 more months so we can claim tranche2
   produce_block( fc::days(185) );

   BOOST_REQUIRE_EQUAL( success(), 
                        claim( N(founder1) ) 
   );

   // balance should be 450.0000 as tranche2 has been reached
   BOOST_REQUIRE_EQUAL( core_sym::from_string("450.0000"), 
                        get_balance(N(founder1))  
   );

   // produce 6 more months so we can claim tranche3
   produce_block( fc::days(185) );

   BOOST_REQUIRE_EQUAL( success(), 
                        claim( N(founder1) ) 
   );
      // balance should be 1000.0000 as tranche3 has been reached
   BOOST_REQUIRE_EQUAL( core_sym::from_string("1000.0000"), 
                        get_balance(N(founder1))  
   );

   BOOST_REQUIRE_EQUAL( core_sym::from_string("9000.0000"), 
                        get_balance(N(founders))  
   );


   std::cout << "head block time: " << last_block_time() << std::endl;
   std::cout << "balance: " << get_balance(N(founder1)) << std::endl;

} FC_LOG_AND_RETHROW()


// update rececipient value
// test existing recipient

BOOST_AUTO_TEST_SUITE_END()
