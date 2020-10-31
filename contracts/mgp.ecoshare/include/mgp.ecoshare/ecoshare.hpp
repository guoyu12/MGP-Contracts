#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>


namespace mgpecoshare {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;

#define BASE_SYMBOL symbol("MGP", 4)
#define BASE_TRANSFER_FROM "eosio.token"_n
#define SYS_ACCOUNT "mgpchain2222"_n
#define SHOP_ACCOUNT "mgpchainshop"_n
#define AGENT_ACCUNT "mgpagentdiya"_n

class [[eosio::contract("mgp.ecoshare")]] mgp_ecoshare: public eosio::contract {
  public:
    using contract::contract;
    mgp_ecoshare(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds) {}
	
	TABLE configs_ {
        name account;
        string burn_memo;
        int destruction;
        bool redeemallow;
        asset minpay;

		uint64_t primary_key() const { return account.value; }
    };
    typedef eosio::multi_index<"configs"_n, configs_> configs;	
	
	TABLE balances_ {
        name account;
        asset remaining;
		uint64_t primary_key() const { return account.value; }
    };
    typedef eosio::multi_index<"balances"_n, balances_> balances;
	
	// 新增额度表
	TABLE quota_ {
        name account;
        asset remaining;
		uint64_t primary_key() const { return account.value; }
    };
    typedef eosio::multi_index<"quota"_n, quota_> quota;
	
	TABLE ethaddressbook_{
        name account;
        string address;

        uint64_t primary_key() const{ return account.value; }
    };
    typedef eosio::multi_index<"addressbook"_n, ethaddressbook_> addressbook;

	ACTION configure( string burn_memo, int destruction, bool redeemallow, asset minpay );
    ACTION bindaddress(name account,string address);
    ACTION delbind(name account,string address);
    ACTION redeem(name account);
	
	void transfer(name from, name to, asset quantity, string memo);
};


extern "C" void apply(uint64_t receiver, uint64_t code, uint64_t action)
{
	if ( code == BASE_TRANSFER_FROM.value && action == "transfer"_n.value)
	{
		eosio::execute_action( eosio::name(receiver), eosio::name(code), &mgp_ecoshare::transfer );
	}
	else if (code == receiver)
	{
		switch (action)
		{
			EOSIO_DISPATCH_HELPER( mgp_ecoshare, (configure)(redeem)(bindaddress)(delbind))
		}
	}
}



}