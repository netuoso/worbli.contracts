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

class worbli_system_tester : public tester {
public:


   void basic_setup() {
      produce_blocks( 2 );

      create_accounts({ N(worbli.admin), N(eosio.token), N(eosio.stake), N(eosio.usage),
                        N(eosio.saving), N(eosio.ppay), N(eosio.rex)});

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
      create( config::system_account_name, asset::from_string("10000000000.0000 " + core_symbol.name()));
      issue(config::system_account_name, config::system_account_name, asset(10000000000000, core_symbol), "" );
      transfer(config::system_account_name, N(worbli.admin), asset(10000000000000, symbol(4,"TST")), "");
      BOOST_REQUIRE_EQUAL( asset(10000000000000, core_symbol), get_balance( "worbli.admin", core_symbol ) );
   }

   void deploy_contract( bool call_init = true,  string sym_str = "4,TST") {
      set_code( config::system_account_name, contracts::system_wasm() );
      set_abi( config::system_account_name, contracts::system_abi().data() );
      if( call_init ) {
         base_tester::push_action(config::system_account_name, N(init),
                                               config::system_account_name,  mutable_variant_object()
                                               ("version", 0)
                                               ("core", sym_str)
         );
      }
      {
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   void deploy_legacy_contract( bool call_init = true ) {
      set_code( config::system_account_name, contracts::util::worbli_system_wasm_old() );
      set_abi( config::system_account_name, contracts::util::worbli_system_abi_old().data() );

      {
         const auto& accnt = control->db().get<account_object,by_name>( config::system_account_name );
         abi_def abi;
         BOOST_REQUIRE_EQUAL(abi_serializer::to_abi(accnt.abi, abi), true);
         abi_ser.set_abi(abi, abi_serializer_max_time);
      }
   }

   enum class setup_level {
      deploy_legacy_contract,
      full
   };

   worbli_system_tester(setup_level l = setup_level::full) {
      basic_setup();

      produce_blocks( 2 );
       
      if( l == setup_level::deploy_legacy_contract ) {
         sym_name = "WBI";
         create_core_token(symbol(SY(4, WBI)));
         deploy_legacy_contract();
         return;
      }

      create_core_token();
      deploy_contract();
   }

   action_result push_system_action( const account_name& signer, const action_name &name, const variant_object &data, bool auth = true ) {
         string action_type_name = abi_ser.get_action_type(name);

         action act;
         act.account = config::system_account_name;
         act.name = name;
         act.data = abi_ser.variant_to_binary( action_type_name, data, abi_serializer_max_time );

         return base_tester::push_action( std::move(act), uint64_t(signer) );
   }

   action_result push_token_action( const account_name& signer, const action_name &name, const variant_object &data ) {
      string action_type_name = token_abi_ser.get_action_type(name);

      action act;
      act.account = N(eosio.token);
      act.name    = name;
      act.data    = token_abi_ser.variant_to_binary( action_type_name, data,abi_serializer_max_time );

      return base_tester::push_action( std::move(act), uint64_t(signer));
   }

   asset get_balance( const account_name& act, symbol balance_symbol = symbol{CORE_SYM} ) {
      vector<char> data = get_row_by_account( N(eosio.token), act, N(accounts), balance_symbol.to_symbol_code().value );
      return data.empty() ? asset(0, balance_symbol) : token_abi_ser.binary_to_variant("account", data, abi_serializer_max_time)["balance"].as<asset>();
   }

   fc::variant get_prodpay( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(prodpay), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant("producer_pay", data, abi_serializer_max_time);
   }

   fc::variant get_stats( const string& symbolname )
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), symbol_code, N(stat), symbol_code );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "currency_stats", data, abi_serializer_max_time );
   }

   fc::variant get_account( account_name acc, const string& symbolname)
   {
      auto symb = eosio::chain::symbol::from_string(symbolname);
      auto symbol_code = symb.to_symbol_code().value;
      vector<char> data = get_row_by_account( N(eosio.token), acc, N(accounts), symbol_code );
      return data.empty() ? fc::variant() : token_abi_ser.binary_to_variant( "account", data, abi_serializer_max_time );
   }

   fc::variant get_delegated_ram( const account_name& from, const account_name& to ) {
      vector<char> data = get_row_by_account( config::system_account_name, from, N(delram), to );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "delegated_ram", data, abi_serializer_max_time );
   }

   fc::variant get_total_stake( const account_name& act ) {
      vector<char> data = get_row_by_account( config::system_account_name, act, N(userres), act );
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "user_resources", data, abi_serializer_max_time );
   }

   fc::variant get_global_state() {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(global), N(global) );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "eosio_global_state", data, abi_serializer_max_time );
   }

   fc::variant get_rammarket(symbol balance_symbol = symbol{CORE_SYM}) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(rammarket), balance_symbol.value() );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "exchange_state", data, abi_serializer_max_time );
   }

   fc::variant get_accountinfo(const account_name& act) {
      vector<char> data = get_row_by_account( config::system_account_name, config::system_account_name, N(accountinfo1), act );
      if (data.empty()) std::cout << "\nData is empty\n" << std::endl;
      return data.empty() ? fc::variant() : abi_ser.binary_to_variant( "account_info", data, abi_serializer_max_time );
   }

   action_result create( account_name issuer,
                asset        maximum_supply ) {

      return push_token_action( N(eosio.token), N(create), mvo()
           ( "issuer", issuer)
           ( "maximum_supply", maximum_supply)
      );
   }

   action_result issue( account_name issuer, account_name to, asset quantity, string memo ) {
      return push_token_action( issuer, N(issue), mvo()
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result retire( account_name issuer, asset quantity, string memo ) {
      return push_token_action( issuer, N(retire), mvo()
           ( "quantity", quantity)
           ( "memo", memo)
      );

   }

   action_result transfer( account_name from,
                  account_name to,
                  asset        quantity,
                  string       memo ) {
      return push_token_action( from, N(transfer), mvo()
           ( "from", from)
           ( "to", to)
           ( "quantity", quantity)
           ( "memo", memo)
      );
   }

   action_result open( account_name owner,
                       const string& symbolname,
                       account_name ram_payer    ) {
      return push_token_action( ram_payer, N(open), mvo()
           ( "owner", owner )
           ( "symbol", symbolname )
           ( "ram_payer", ram_payer )
      );
   }

   action_result close( account_name owner,
                        const string& symbolname ) {
      return push_token_action( owner, N(close), mvo()
           ( "owner", owner )
           ( "symbol", "0,CERO" )
      );
   }

   void create_accounts_with_resources( vector<account_name> accounts, account_name creator = config::system_account_name ) {
      for( auto a : accounts ) {
         create_account_with_resources( a, creator );
      }
   }

   transaction_trace_ptr create_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( config::system_account_name, N(buyrambytes), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("payer", creator)
                                            ("receiver", a)
                                            ("bytes", ram_bytes) )
                              );
      trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", asset::from_string("10.0000 " + sym_name))
                                            ("stake_cpu_quantity", asset::from_string("10.0000 " + sym_name))
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   transaction_trace_ptr create_free_account_with_resources( account_name a, account_name creator, uint32_t ram_bytes = 8000 ) {
      signed_transaction trx;
      set_transaction_headers(trx);

      authority owner_auth;
      owner_auth =  authority( get_public_key( a, "owner" ) );

      trx.actions.emplace_back( vector<permission_level>{{creator,config::active_name}},
                                newaccount{
                                   .creator  = creator,
                                   .name     = a,
                                   .owner    = owner_auth,
                                   .active   = authority( get_public_key( a, "active" ) )
                                });

      trx.actions.emplace_back( get_action( config::system_account_name, N(delegateram), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("bytes", ram_bytes) 
                                            ("ram_bytes", ram_bytes))
                              );
      trx.actions.emplace_back( get_action( config::system_account_name, N(delegatebw), vector<permission_level>{{creator,config::active_name}},
                                            mvo()
                                            ("from", creator)
                                            ("receiver", a)
                                            ("stake_net_quantity", asset::from_string("10.0000 " + sym_name))
                                            ("stake_cpu_quantity", asset::from_string("10.0000 " + sym_name))
                                            ("transfer", 0 )
                                          )
                                );

      set_transaction_headers(trx);
      trx.sign( get_private_key( creator, "active" ), control->get_chain_id()  );
      return push_transaction( trx );
   }

   action_result claimrewards( const account_name account ) {
      return push_system_action( account, N(claimrewards), mvo()
           ( "owner", account)
      );
   }

   action_result sellram( const account_name account, uint64_t numbytes ) {
      return push_system_action( account, N(sellram), mvo()
           ( "account", account)
           ( "bytes", numbytes)
      );
   }

   action_result delegateram( const account_name from, const account_name to, uint64_t numbytes ) {
      return push_system_action( from, N(delegateram), mvo()
           ("from", from)
           ("receiver", to)
           ("bytes", numbytes)
      );
   }

   action_result buyram( const account_name from, const account_name to, uint64_t numbytes ) {
      return push_system_action( from, N(buyrambytes), mvo()
           ("payer", from)
           ("receiver", to)
           ("bytes", numbytes)
      );
   }

   action_result rmvproducer( const account_name producer ) {
      return push_system_action( N(worbli.admin), N(rmvproducer), mvo()
           ("producer", producer)
      );
   }

   action_result addprod( const account_name producer ) {
      return push_system_action( N(worbli.admin), N(addproducer), mvo()
           ("producer", producer)
      );
   }

   action_result regprod( const account_name producer ) {
      return push_system_action( producer, N(regproducer), mvo()
           ("producer", producer)
           ("producer_key", get_public_key( producer, "active" ))
           ("url", "http://example.com")
           ("location", 844)
      );
   }

   action_result unregprod( const account_name producer ) {
      return push_system_action( producer, N(unregprod), mvo()
           ("producer", producer)
      );
   }

   action_result activate() {
      return push_system_action( N(eosio), N(togglesched), mvo()
           ("is_active", 1)
      );
   }

   action_result setram(uint64_t max_ram_size) {
      return push_system_action( N(eosio), N(setram), mvo()
           ("max_ram_size", max_ram_size)
      );
   }

   abi_serializer abi_ser;
   abi_serializer token_abi_ser;
   string sym_name = string("TST");
};

BOOST_AUTO_TEST_SUITE(worbli_system_tests)

BOOST_FIXTURE_TEST_CASE( delegate_ram_tests, worbli_system_tester ) try {
      //deploy_contract();
      issue(config::system_account_name, N(eosio), asset(10000000000000, symbol(4,"TST")), "" );
      create_free_account_with_resources(N(test1), N(worbli.admin));
      BOOST_REQUIRE_EQUAL( core_sym::from_string("20.0000"), get_balance("eosio.stake"));
      BOOST_REQUIRE_EQUAL( core_sym::from_string("999999980.0000"), get_balance("worbli.admin"));

      REQUIRE_MATCHING_OBJECT( get_total_stake(N(test1)), mvo()
         ("owner", "test1")
         ("net_weight", "10.0000 TST")
         ("cpu_weight", "10.0000 TST")
         ("ram_stake", "235.2941 TST")
         ("ram_bytes", "8000")
      );

      BOOST_REQUIRE_EQUAL(wasm_assert_msg( "insufficient quota" ),
                           sellram(N(test1), 1));

      delegateram(N(eosio), N(test1), 8000);
      BOOST_REQUIRE_EQUAL( core_sym::from_string("1000000000.0000"), get_balance("eosio"));

      auto delram = get_delegated_ram( N(eosio), N(test1) );

      REQUIRE_MATCHING_OBJECT( get_total_stake(N(test1)), mvo()
         ("owner", "test1")
         ("net_weight", "10.0000 TST")
         ("cpu_weight", "10.0000 TST")
         ("ram_stake", "470.5882 TST")
         ("ram_bytes", "16000")
      );

      BOOST_REQUIRE_EQUAL(wasm_assert_msg( "insufficient quota" ),
                           sellram(N(test1), 1));

      auto intital_eosio_stake_bal = get_balance("eosio.stake");
      auto intital_test1_ram_stake = get_total_stake(N(test1))["ram_stake"].as<asset>();

      buyram(N(worbli.admin), N(test1), 1000);
      transfer(N(worbli.admin), N(test1), asset(50000000, symbol(4,"TST")), "");
      
      auto eosio_stake_bal = get_balance("eosio.stake");
      auto test1_ram_stake = get_total_stake(N(test1))["ram_stake"].as<asset>();

      BOOST_REQUIRE_EQUAL(eosio_stake_bal - intital_eosio_stake_bal,
                          test1_ram_stake - intital_test1_ram_stake);

      produce_blocks( 100 );

      BOOST_REQUIRE_EQUAL( success(), sellram(N(test1), 1) );
      BOOST_REQUIRE_EQUAL( success(), sellram(N(test1), 998) );

     BOOST_REQUIRE_EQUAL(wasm_assert_msg( "insufficient quota" ),
                          sellram(N(test1), 1));
      

} FC_LOG_AND_RETHROW()

/**
// can remove after new contract is deployed
BOOST_FIXTURE_TEST_CASE( update_system_contract, worbli_system_tester ) try {
      worbli_system_tester t(worbli_system_tester::setup_level::deploy_legacy_contract);
      vector<account_name> producers = {N(prod1), N(prod2), N(prod3), N(prod4),
                                      N(prod5), N(prod11), N(prod12), N(prod13),
                                      N(prod14), N(prod15), N(prod21), N(prod22),
                                      N(prod23), N(prod24), N(prod25), N(prod31),
                                      N(prod32), N(prod33), N(prod34), N(prod35),
                                      N(prod41)};

      t.create_accounts_with_resources(producers, N(worbli.admin));

      for( auto p : producers ) {
         BOOST_REQUIRE_EQUAL( success(), t.addprod( p ));
         BOOST_REQUIRE_EQUAL( success(), t.regprod( p ));
      }

      BOOST_REQUIRE_EQUAL( success(), t.activate());
      t.produce_blocks(1000);
      
      auto producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);
      
      t.create_free_account_with_resources(N(test1), N(worbli.admin));  

      auto legacy_ram_stake = t.get_total_stake(N(test1))["ram_stake"];

      t.deploy_contract(true, "4,WBI");
      t.produce_blocks(1000); 
 
      t.create_free_account_with_resources(N(test2), N(worbli.admin));      
      auto new_ram_stake = t.get_total_stake(N(test1))["ram_stake"];

      BOOST_REQUIRE_EQUAL( legacy_ram_stake, new_ram_stake);

      BOOST_REQUIRE_EQUAL( success(), t.unregprod(N(prod1)));
      t.produce_blocks(1200);

      producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod11", producer_keys[0].producer_name);

      BOOST_REQUIRE_EQUAL( success(), t.regprod(N(prod1)));
      t.produce_blocks(1200);

      producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);      

} FC_LOG_AND_RETHROW()

// can remove after new contract is deployed
BOOST_FIXTURE_TEST_CASE( update_inflation_producer_pay, worbli_system_tester ) try {
      
      // setup legacy tester
      worbli_system_tester t(worbli_system_tester::setup_level::deploy_legacy_contract);
      vector<account_name> producers = {N(prod1), N(prod2), N(prod3), N(prod4),
                                      N(prod5), N(prod11), N(prod12), N(prod13),
                                      N(prod14), N(prod15), N(prod21), N(prod22),
                                      N(prod23), N(prod24), N(prod25), N(prod31),
                                      N(prod32), N(prod33), N(prod34), N(prod35),
                                      N(prod41)};

      t.create_accounts_with_resources(producers, N(worbli.admin));

      for( auto p : producers ) {
         BOOST_REQUIRE_EQUAL( success(), t.addprod( p ));
         BOOST_REQUIRE_EQUAL( success(), t.regprod( p ));
      }

      BOOST_REQUIRE_EQUAL( success(), t.activate());
      BOOST_REQUIRE_EQUAL( success(), t.setram(68719476736));
      t.produce_blocks(1000);
      
      auto producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);

      BOOST_REQUIRE( bool(t.create_free_account_with_resources(N(test1), N(worbli.admin))));
      auto legacy_ram_stake = t.get_total_stake(N(test1))["ram_stake"].as<asset>().get_amount();

      // setup new tester
      create_accounts_with_resources(producers, N(worbli.admin));

      for( auto p : producers ) {
         BOOST_REQUIRE_EQUAL( success(), addprod( p ));
         BOOST_REQUIRE_EQUAL( success(), regprod( p ));
      }
      BOOST_REQUIRE_EQUAL( success(), activate());
      BOOST_REQUIRE( bool(create_free_account_with_resources(N(test2), N(worbli.admin))));

      produce_blocks(1001);
      
      BOOST_REQUIRE( !is_same_chain(t) );
      // trigger inflation calculaiton for both chains      
      produce_blocks( 200 );
      produce_block( fc::days(1) );

      t.produce_blocks( 200 );
      t.produce_block( fc::days(1) );

      producer_keys = control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);   

      auto new_ram_stake = get_total_stake(N(test2))["ram_stake"].as<asset>().get_amount();   
      BOOST_REQUIRE_EQUAL( legacy_ram_stake, new_ram_stake);

      // test inflation calculations
      BOOST_REQUIRE( get_balance(N(eosio.usage)).get_amount() > 0 );
      BOOST_REQUIRE_EQUAL( get_balance(N(eosio.usage)).get_amount(), 
                           t.get_balance(N(eosio.usage), symbol(4,"WBI")).get_amount());

      BOOST_REQUIRE( get_balance(N(eosio.saving)).get_amount() > 0 );
      BOOST_REQUIRE_EQUAL( get_balance(N(eosio.saving)).get_amount(), 
                           t.get_balance(N(eosio.saving), symbol(4,"WBI")).get_amount());

      BOOST_REQUIRE( get_balance(N(eosio.ppay)).get_amount() > 0 );
      BOOST_REQUIRE_EQUAL( get_balance(N(eosio.ppay)).get_amount(), 
                           t.get_balance(N(eosio.ppay), symbol(4,"WBI")).get_amount());


      // test producer pay
      BOOST_REQUIRE_EQUAL( success(), claimrewards(N(prod1)));
      BOOST_REQUIRE_EQUAL(wasm_assert_msg( "producer pay request not found" ),
                          claimrewards(N(prod1)));


} FC_LOG_AND_RETHROW()

// Used to test the listproducer fix.  This can be removed now.
BOOST_FIXTURE_TEST_CASE( test_fix_list_producers, worbli_system_tester ) try {
      worbli_system_tester t(worbli_system_tester::setup_level::deploy_legacy_contract);
      vector<account_name> producers = {N(prod1), N(prod2), N(prod3), N(prod4),
                                      N(prod5), N(prod11), N(prod12), N(prod13),
                                      N(prod14), N(prod15), N(prod21), N(prod22),
                                      N(prod23), N(prod24), N(prod25), N(prod31),
                                      N(prod32), N(prod33), N(prod34), N(prod35),
                                      N(prod41)};

      // setup legacy system contract
      t.create_accounts_with_resources(producers, N(worbli.admin));

      for( auto p : producers ) {
         BOOST_REQUIRE_EQUAL( success(), t.addprod( p ));
         BOOST_REQUIRE_EQUAL( success(), t.regprod( p ));
      }

      BOOST_REQUIRE_EQUAL( success(), t.activate());
      t.produce_blocks(1000);
      
      auto producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);
       
      // deploy new contract
      t.deploy_contract(true, "4,WBI");
      t.produce_blocks(1000); 

      //remove and add producers to populate secondary index
      for( auto p : producers ) {
         BOOST_REQUIRE_EQUAL( success(), t.unregprod( p ));
         BOOST_REQUIRE_EQUAL( success(), t.rmvproducer( p ));
         BOOST_REQUIRE_EQUAL( success(), t.addprod( p ));
         BOOST_REQUIRE_EQUAL( success(), t.regprod( p ));
      }     

      BOOST_REQUIRE_EQUAL( success(), t.unregprod(N(prod1)));
      t.produce_blocks(1200);

      producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod11", producer_keys[0].producer_name);

      BOOST_REQUIRE_EQUAL( success(), t.regprod(N(prod1)));
      t.produce_blocks(1200);

      producer_keys = t.control->head_block_state()->active_schedule.producers;
      BOOST_REQUIRE_EQUAL( "prod1", producer_keys[0].producer_name);

} FC_LOG_AND_RETHROW()
**/

BOOST_FIXTURE_TEST_CASE( test_account_info, worbli_system_tester ) try {
   create_account_with_resources(N(parent1), N(worbli.admin));
   transfer(N(worbli.admin), N(parent1), asset(50000000, symbol(4,"TST")), "");

   // global max_subaccounts = 0 and user max_subaccounts = -1 (defer to global)
   // attempt should fail
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources(N(child1), N(parent1)),
                            eosio_assert_message_exception, eosio_assert_message_is("subaccount limit reached"));

   // set global limit to 1 subaccount
   BOOST_REQUIRE_EQUAL( success(), push_system_action( N(worbli.admin), N(setwparams), mvo() ("max_subaccounts", 1)));
   create_account_with_resources(N(child1), N(parent1));

   // 2nd subaccount should fail
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources(N(child2), N(parent1)),
                            eosio_assert_message_exception, eosio_assert_message_is("subaccount limit reached"));

   // child not allowed to create subaccount (kyc = 0)
   transfer(N(worbli.admin), N(child1), asset(50000000, symbol(4,"TST")), "");

   REQUIRE_MATCHING_OBJECT( get_accountinfo(N(child1)), mvo()
      ("account", "child1")
      ("parent", "parent1")
      ("kyc", 0)
      ("max_subaccounts", -1)
   );

   BOOST_REQUIRE_EXCEPTION( create_account_with_resources(N(child1child), N(child1)),
                            eosio_assert_message_exception, eosio_assert_message_is("not permitted to create subaccounts"));

   // test user specific limits
   // set parent1 specific limit = 2
   BOOST_REQUIRE_EQUAL( success(), push_system_action( N(worbli.admin), N(updacctinfo), mvo()
                                       ("account", "parent1")
                                       ("kyc", 1)
                                       ("max_subaccounts", 2)
                                    )
                     );
   // set global limit = 1 parent 1 limit = 2.  parent 1 has one subaccount already
   // this should succeed
   create_account_with_resources(N(child2), N(parent1));

   BOOST_REQUIRE_EXCEPTION( create_account_with_resources(N(child3), N(parent1)),
                            eosio_assert_message_exception, eosio_assert_message_is("subaccount limit reached"));

   // set global max_subaccounts = 3
   BOOST_REQUIRE_EQUAL( success(), push_system_action( N(worbli.admin), N(setwparams), mvo() ("max_subaccounts", 3)));

   // should still fail. When account specific limit is set it supercedes global
   BOOST_REQUIRE_EXCEPTION( create_account_with_resources(N(child3), N(parent1)),
                            eosio_assert_message_exception, eosio_assert_message_is("subaccount limit reached"));

   // confirm same behavior when eosio creates accounts.
   transfer(N(worbli.admin), N(eosio), asset(50000000, symbol(4,"TST")), "");
   create_account_with_resources(N(parent2), N(eosio));
   create_account_with_resources(N(parent3), N(eosio));
   create_account_with_resources(N(parent4), N(eosio));
   create_account_with_resources(N(parent5), N(eosio));

} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()
