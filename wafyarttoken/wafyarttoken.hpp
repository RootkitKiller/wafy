/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

using namespace eosio;
using std::string;

class wafyarttoken : public contract
{
  using contract::contract;

public:
  wafyarttoken(account_name self) : contract(self) {}

  // @abi action
  void create(account_name issuer,
              asset maximum_supply);

  // @abi action
  void issue(account_name to, asset quantity, string memo);

  // @abi action
  void transfer(account_name from,
                account_name to,
                asset quantity,
                string memo);

  // @abi action
  void staketoken(account_name byname,asset quantity);
  // @abi action
  void addtoken(account_name byname,uint64_t amount);

  inline asset get_supply(symbol_name sym) const;

  inline asset get_balance(account_name owner, symbol_name sym) const;

private:

  symbol_type MZSYMBOL=symbol_type(string_to_symbol(4, "MZ"));;
  uint64_t getmzbal(account_name byname);

  // @abi table accounts i64
  struct account
  {
    asset balance;

    uint64_t primary_key() const { return balance.symbol.name(); }
  };
  typedef eosio::multi_index<N(accounts), account> accounts;

  struct currency_stats
  {
    asset supply;
    asset max_supply;
    account_name issuer;

    uint64_t primary_key() const { return supply.symbol.name(); }
  };
  
  typedef eosio::multi_index<N(stat), currency_stats> stats;

  void sub_balance(account_name owner, asset value);
  void add_balance(account_name owner, asset value, account_name ram_payer);

public:
  struct transfer_args
  {
    account_name from;
    account_name to;
    asset quantity;
    string memo;
  };
};
asset wafyarttoken::get_supply(symbol_name sym) const
{
  stats statstable(_self, sym);
  const auto &st = statstable.get(sym);
  return st.supply;
}

asset wafyarttoken::get_balance(account_name owner, symbol_name sym) const
{
  accounts accountstable(_self, owner);
  const auto &ac = accountstable.get(sym);
  return ac.balance;
}
