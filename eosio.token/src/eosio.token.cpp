/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */

#include <eosio.token/eosio.token.hpp>
#include <eosiolib/print.hpp>

namespace eosio {

   void token::create( name   issuer,
                     asset  maximum_supply )
   {
      require_auth( _self );

      auto sym = maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( _self, [&]( auto& s ) {
         s.supply.symbol = maximum_supply.symbol;
         s.max_supply    = maximum_supply;
         s.issuer        = issuer;
      });
   }


   void token::issue( name to, asset quantity, string memo )
   {
      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      require_auth( st.issuer );
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must issue positive quantity" );

      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify( st, same_payer, [&]( auto& s ) {
         s.supply += quantity;
      });

      add_balance( st.issuer, quantity, st.issuer );

      if( to != st.issuer ) {
         SEND_INLINE_ACTION( *this, transfer, { {st.issuer, "active"_n} },
                           { st.issuer, to, quantity, memo }
         );
      }
   }

   void token::retire( asset quantity, string memo )
   {
      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      stats statstable( _self, sym.code().raw() );
      auto existing = statstable.find( sym.code().raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist" );
      const auto& st = *existing;

      require_auth( st.issuer );
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must retire positive quantity" );

      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );

      statstable.modify( st, same_payer, [&]( auto& s ) {
         s.supply -= quantity;
      });

      sub_balance( st.issuer, quantity );
   }

   void token::transfer( name    from,
                        name    to,
                        asset   quantity,
                        string  memo )
   {
      is_transfer_allowed( from, to, quantity );
      eosio_assert( from != to, "cannot transfer to self" );
      require_auth( from );
      eosio_assert( is_account( to ), "to account does not exist");
      auto sym = quantity.symbol.code();
      stats statstable( _self, sym.raw() );
      const auto& st = statstable.get( sym.raw() );

      require_recipient( from );
      require_recipient( to );

      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      auto payer = has_auth( to ) ? to : from;

      sub_balance( from, quantity );
      add_balance( to, quantity, payer );
   }

   void token::sub_balance( name owner, asset value ) {
      accounts from_acnts( _self, owner.value );

      const auto& from = from_acnts.get( value.symbol.code().raw(), "no balance object found" );
      eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

      from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
         });
   }

   void token::add_balance( name owner, asset value, name ram_payer )
   {
      accounts to_acnts( _self, owner.value );
      auto to = to_acnts.find( value.symbol.code().raw() );
      if( to == to_acnts.end() ) {
         to_acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = value;
         });
      } else {
         to_acnts.modify( to, same_payer, [&]( auto& a ) {
         a.balance += value;
         });
      }
   }

   void token::open( name owner, const symbol& symbol, name ram_payer )
   {
      require_auth( ram_payer );

      auto sym_code_raw = symbol.code().raw();

      stats statstable( _self, sym_code_raw );
      const auto& st = statstable.get( sym_code_raw, "symbol does not exist" );
      eosio_assert( st.supply.symbol == symbol, "symbol precision mismatch" );

      accounts acnts( _self, owner.value );
      auto it = acnts.find( sym_code_raw );
      if( it == acnts.end() ) {
         acnts.emplace( ram_payer, [&]( auto& a ){
         a.balance = asset{0, symbol};
         });
      }
   }

   void token::close( name owner, const symbol& symbol )
   {
      require_auth( owner );
      accounts acnts( _self, owner.value );
      auto it = acnts.find( symbol.code().raw() );
      eosio_assert( it != acnts.end(), "Balance row already deleted or never existed. Action won't have any effect." );
      eosio_assert( it->balance.amount == 0, "Cannot close because the balance is not zero." );
      acnts.erase( it );
   }

   void token::setcntlr( const symbol_code sym, name controller )
   {
      stats statstable( _self, sym.raw() );
      auto existing = statstable.find( sym.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting controller" );
      const auto& st = *existing;

      require_auth( st.issuer );
      // verify controller is a contract

      controller_table controllers( _self, sym.raw() );
      auto c_itr = controllers.upper_bound(0);

      if( c_itr != controllers.end() ) {
         controllers.erase( c_itr );
      }

      controllers.emplace( _self, [&]( auto& c ) {
         c.controller = controller;
      });
   }

   void token::unsetcntlr( const symbol_code sym )
   {
      stats statstable( _self, sym.raw() );
      auto existing = statstable.find( sym.raw() );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before setting controller" );
      const auto& st = *existing;

      require_auth( st.issuer );
      // verify controller is a contract

      controller_table controllers( _self, sym.raw() );
      auto c_itr = controllers.upper_bound(0);

      if( c_itr != controllers.end() ) {
         controllers.erase( c_itr );
      }
   }

   void token::is_receiver_verified( name from, name controller, symbol_code sym ) {
      receivereqs_table requirements( controller, sym.raw() );
      
      auto itr = requirements.upper_bound(0);
      if( itr != requirements.end() ) {
         registry_table registries( controller, sym.raw() );
         for( const auto& req : requirements ) {
            print("requirement: ", req.key, "\n");
         }
      }

   }

   void token::is_sender_verified( name from,  name controller, symbol_code sym ) {
      sendreqs_table requirements( controller, sym.raw() );
      
      auto req_itr = requirements.upper_bound(0);      
      if( req_itr != requirements.end() ) {
         registry_table registries( controller, sym.raw() );
         auto registry_itr = registries.upper_bound(0);
         eosio_assert( registry_itr != registries.end(), "No registry for thi symbol" );

         for( const auto& reqistry : registries ) {
            // lookup permissions
            print("requirement: ", reqistry.registry, "\n");
         }
      }
   }


   void token::is_transfer_allowed( name from, name to, asset quantity ) {
      controller_table controllertable( _self, quantity.symbol.code().raw() );
      auto ctrlitr = controllertable.upper_bound(0);

      if( ctrlitr != controllertable.end() ) {
         // add condition threshold for requirements
         is_receiver_verified( from, ctrlitr->controller, quantity.symbol.code() );
         is_sender_verified( from, ctrlitr->controller, quantity.symbol.code() );
      }
   }


   void token::verify( name from, sendreqs_table& reqs) {
      
   }


} /// namespace eosio

EOSIO_DISPATCH( eosio::token, (create)(issue)(transfer)(open)(close)(retire)(setcntlr)(unsetcntlr) )
