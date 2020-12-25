#pragma once

#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/transaction.hpp>
#include <eosio/system.hpp>
#include <eosio/crypto.hpp>
#include <eosio/action.hpp>
#include <string>

#include "wasm_db.hpp"
#include "bpvoting_entities.hpp"

using namespace wasm::db;

namespace mgp {

using eosio::asset;
using eosio::check;
using eosio::datastream;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::unsigned_int;

using std::string;

static constexpr bool DEBUG = true;

#define WASM_FUNCTION_PRINT_LENGTH 50

#define MGP_LOG( debug, exception, ... ) {  \
if ( debug ) {                               \
   std::string str = std::string(__FILE__); \
   str += std::string(":");                 \
   str += std::to_string(__LINE__);         \
   str += std::string(":[");                \
   str += std::string(__FUNCTION__);        \
   str += std::string("]");                 \
   while(str.size() <= WASM_FUNCTION_PRINT_LENGTH) str += std::string(" ");\
   eosio::print(str);                                                             \
   eosio::print( __VA_ARGS__ ); }}

class [[eosio::contract("mgp.otcstore")]] mgp_otcstore: public eosio::contract {
  private:
    global_singleton    _global;
    global_t            _gstate;
    dbc                 _dbc;

  public:
    using contract::contract;
    mgp_otcstore(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds), _global(get_self(), get_self().value), _dbc(get_self())
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~mgp_otcstore() {
        _global.set( _gstate, get_self() );
    }

    [[eosio::action]]
    void init();  //only code maintainer can init

    [[eosio::action]]
    void config(const uint64_t& max_tally_vote_iterate_steps,
                const uint64_t& max_tally_unvote_iterate_steps,
                const uint64_t& max_reward_iterate_steps,
                const uint64_t& max_bp_size,
                const uint64_t& election_round_sec,
                const uint64_t& refund_delay_sec,
                const uint64_t& election_round_start_hour,
                const asset& min_bp_list_quantity,
                const asset& min_bp_accept_quantity,
                const asset& min_bp_vote_quantity);

    [[eosio::action]]
    void setexecround(const uint64_t& execution_round);

    [[eosio::action]]
    void unvote(const name& owner, const uint64_t vote_id);

    [[eosio::action]]
    void execute(); //anyone can invoke, but usually by the platform

    [[eosio::action]]
    void delist(const name& issuer); //candidate to delist self

    [[eosio::action]]
    void resetvotes(const uint64_t round_id) {
      require_auth( _self );

		  election_round_t round(round_id);
      check( _dbc.get(round), "round not exist: " + to_string(round_id) );

      for (auto& bp : round.elected_bps) {
        candidate_t candidate(bp.first);
        check( _dbc.get(candidate), "candidate not found: " + bp.first.to_string() );

        candidate.tallied_votes.amount = 0;
        _dbc.set( candidate );
      }
    }

    [[eosio::action]]
    void setcandidate(const name& owner, const uint64_t share,
      const asset& staked_votes,
      const asset& received_votes,
      const asset& tallied_votes,
      const asset& last_claimed_rewards,
      const asset& total_claimed_rewards,
      const asset& unclaimed_rewards) {

      require_auth( _self );
      candidate_t candidate(owner);
      check( _dbc.get(candidate), "candidate not found: " + owner.to_string() );

      candidate.self_reward_share     = share;
      candidate.staked_votes          = staked_votes;
      candidate.received_votes        = received_votes;
      candidate.tallied_votes         = tallied_votes;
      candidate.last_claimed_rewards  = last_claimed_rewards;
      candidate.total_claimed_rewards = total_claimed_rewards;
      candidate.unclaimed_rewards     = unclaimed_rewards;

      _dbc.set( candidate );

    }

    [[eosio::action]]
    void claimrewards(const name& issuer, const bool is_voter); //voter/candidate to claim rewards

    [[eosio::on_notify("eosio.token::transfer")]]
    void deposit(name from, name to, asset quantity, string memo);

    using init_action     = action_wrapper<name("init"),      &mgp_bpvoting::init     >;
    using config_action   = action_wrapper<name("config"),    &mgp_bpvoting::config   >;

    using unvote_action   = action_wrapper<name("unvote"),    &mgp_bpvoting::unvote   >;
    using execute_action  = action_wrapper<name("execute"),   &mgp_bpvoting::execute  >;
    using delist_action   = action_wrapper<name("delist"),    &mgp_bpvoting::delist   >;
    using transfer_action = action_wrapper<name("transfer"),  &mgp_bpvoting::deposit  >;

    using setexecround_action = action_wrapper<name("setexecround"),  &mgp_bpvoting::setexecround >;
    using resetvotes_action = action_wrapper<name("resetvotes"), &mgp_bpvoting::resetvotes >;
    using setcandidate_action = action_wrapper<name("setcandidate"), &mgp_bpvoting::setcandidate >;

  private:
    uint64_t get_round_id(const time_point& ct);

    void _list(const name& owner, const asset& quantity, const uint32_t& voter_reward_share_percent);
    void _vote(const name& owner, const name& target, const asset& quantity);
    void _elect(election_round_t& round, const candidate_t& candidate);
    void _current_election_round(const time_point& ct, election_round_t& election_round);
    void _tally_votes(election_round_t& last_round, election_round_t& execution_round);
    void _apply_unvotes(election_round_t& round);
    void _allocate_rewards(election_round_t& round);
    void _execute_rewards(election_round_t& round);

};

inline vector <string> string_split(string str, char delimiter) {
      vector <string> r;
      string tmpstr;
      while (!str.empty()) {
          int ind = str.find_first_of(delimiter);
          if (ind == -1) {
              r.push_back(str);
              str.clear();
          } else {
              r.push_back(str.substr(0, ind));
              str = str.substr(ind + 1, str.size() - ind - 1);
          }
      }
      return r;

  }

}