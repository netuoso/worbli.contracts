# worbli.contracts



Following is a description of the changes made to the code EOSIO system contracts.

For details on the base eosio.system contract please see [eosio.system](https://github.com/EOSIO/eosio.contracts)

The Worbli modifications to the core EOSIO system contracts are described here [worbli.contracts](./worbli.contracts.md)

## Producer Management

### addprod( const name producer )
Caller: admin

Adds an account to the producers table which makes them a reserve.  This will allow a producer to be voted for but they will not be able to regprod until they have been promoted by an admin.

### promoteprod( const name producer )
Prerequisite: **addprod** has been called on this account
Caller: admin

Promotes a producer to active which will allow the producer to call ***regproducer*** and start making blocks.

### regproducer( const name& producer, const public_key& producer_key, const std::string& url, uint16_t location )
Prerequisite: **promoteprod** has been called on this account
Caller: producer

Adds the producer to the schedule.

### demoteprod( const name producer )
Caller: admin

Moves a primary to reserve.

### unregprod( const name& producer )
Caller: producer

Removes the producer from the schedule.

### rmvprod( const name& producer )
Caller: admin

Removes the producer from the producers table.  

Todo: Determine what to do with votes

## Account Management

### newaccount()

***modifications:***

* admin and eosio have no restrictions
* user need to pass identity and subaccount limit checks
* removed name bidding
* record account relationships in multindex

### delegateram( name from, name receiver, int64_t bytes )

Allows admin or eosio to delegate (lend RAM) to other accounts. Undelegate is is not implemented

### buyram









