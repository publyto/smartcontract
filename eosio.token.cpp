/* Copyright (C) Publyto, Inc - All Rights Reserved
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Publyto, December 2018
 */

#include "eosio.token.hpp"

namespace eosio {
	
void token::check(account_name euser, account_name iuser, string memo){
	require_auth(_self);
	eosio_assert(is_account(euser), "user account does not exist");
	
	maptbl maptable(_self, _self);
	auto iter = maptable.find(euser);	
	eosio_assert(iter != maptable.end(), "external account did not exist");
	
	pubtbl pubtable(_self, iuser);
	auto iter2 = pubtable.find(iuser);
	
	pubtable.modify(iter2, _self, [&]( auto& pubtable ) {
		pubtable.eos_account = euser;
	});
	
	if(iter2->balance.amount > 0){
		itransfer(N(publytoken11), euser, iter2->balance,"link internal account to external account");
		draw(iuser, iter2->balance);
	}
	
	//change connection table status
	
	contbl2 contable(_self, iuser);
	auto iter3 = contable.find(euser);
	contable.modify(contable.begin(), _self, [&]( auto& contable ) {
		contable.status = 2;
	});
}
	
void token::prepare(account_name euser, account_name iuser, string memo){
	require_auth(euser);
	eosio_assert(is_account(euser), "user account does not exist");
	//assumption : iuser always exists because he can call this when he logged in
	//transfer all PUB from iuser to euser (get internal asset and calling pub transfer)
	//asset balance = get_ibalance(iuser)
	//pubtransfer(iuser, 1, euser, 0, balance, memo);
	maptbl maptable(_self, _self);
	auto iter = maptable.find(euser);
	
	eosio_assert(iter == maptable.end(), "external account already exist");
	
	maptable.emplace(_self, [&]( auto& maptable){
		maptable.iuser = iuser;
		maptable.euser = euser;
	});
	
	//making connection status table
	contbl2 contable(_self, iuser);
	auto iter3 = contable.find(iuser);
	eosio_assert(iter3 == contable.end(), "external account already exist");
	if(iter3 == contable.end()){
		contable.emplace(_self, [&]( auto& contable){
			contable.user = euser;
			contable.status = 1;
		});
	}else{
		contable.modify(iter3, _self, [&]( auto& contable ) {
			contable.status = 1;
		});
	}
}

	//internal account should have scope with "iuser"
void token::newaccount(account_name iuser){
	pubtbl pubtable(_self, iuser);
	auto iter = pubtable.find(iuser);
	
	eosio_assert(iter == pubtable.end(), "account already exist");
	
	pubtable.emplace(_self, [&]( auto& pubtable){
		pubtable.user = iuser;
		pubtable.eos_account = N("");
		pubtable.balance = asset(0, eosio::symbol_type(eosio::string_to_symbol(4, "PUB")));
		pubtable.ink = asset(0, eosio::symbol_type(eosio::string_to_symbol(4, "INK")));			
	});
}
	void token::delaccount(account_name euser){
		require_auth(_self);
		maptbl maptable(_self, _self);
		auto iter = maptable.find(euser);
		
		eosio_assert(iter != maptable.end(), "nothing to delete");
		
		maptable.erase(iter);
	}
	
	
	void token::thanks(account_name user, asset quantity, string boardid){
		//permission check
		require_auth( _self );
		//check precondition
		pubtbl pubtable(_self, user);
		auto iter = pubtable.find(user);
		eosio_assert(iter != pubtable.end(), "account does not exist");
		
		eosio_assert(iter->ink.amount >= quantity.amount, "overdrawn balance");
		eosio_assert(quantity.amount > 0, "must transfer positive quantity");
		
		//decrease ink power from pubtable
		pubtable.modify(iter, _self, [&]( auto& pubtable ) {
			pubtable.ink -= quantity;
		});
	}
	
	void token::update(account_name user, asset quantity){
		//permission check
		require_auth( _self );
		//check precondition
		pubtbl pubtable(_self, user);
		auto iter = pubtable.find(user);
		eosio_assert(iter != pubtable.end(), "account does not exist");
		
		//decrease ink power from pubtable
		pubtable.modify(iter, _self, [&]( auto& pubtable ) {
			pubtable.ink = quantity;
		});
	}
	
	void token::refund(account_name from, account_name user){
		//check whether unstake is done or not.
		//if done, erase unstake row
		//move balance to the owner by using pubtransfer)
		unstaketbl unstake_table(_self, from);
		auto iter = unstake_table.find(user);
		
		pubtbl pubtable(_self, from);
		auto iter2 = pubtable.find(from);
		if(iter2->eos_account == N("")){
			//internal to internal transfer
			save(from, iter->balance);			
		}else{
			//internal to external transfer
			//pubtransfer(N(publytoken11), 1, iter2->eos_account, 0, iter->balance, "refund");
			itransfer(N(publytoken11), iter2->eos_account , iter->balance, "refund");
		}
		//delete unstake table row
		unstake_table.erase(iter);
		
	}
	
	void token::pubtransfer(account_name from, bool internalfrom, account_name to, bool internalto, asset balance, string memo){
		require_auth( _self );
		
		//for any internal account, check existence of external then change internalfrom or internalto value
		
		//Internal to internal case
		if(internalfrom == 1 && internalto == 1){
			//account validation
			pubtbl pubtable(_self, from);
			auto iter = pubtable.find(from);
			eosio_assert(iter != pubtable.end(), "from account does not exist");
			pubtbl pubtable2(_self, to);
			auto iter2 = pubtable2.find(to);
			eosio_assert(iter2 != pubtable2.end(), "to account does not exist");
			
			if(iter->eos_account != N("")){
				if(iter2->eos_account != N("")){
					itransfer(iter->eos_account, N(publytoken11), balance, memo);
					INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                            { N(publytoken11), iter2->eos_account, balance, memo } );
					
				}else{
					itransfer(iter->eos_account, N(publytoken11), balance, memo);
					save(to, balance);
				}
			}else{
				if(iter2->eos_account != N("")){
					draw(from, balance);
					INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                             { N(publytoken11), iter2->eos_account, balance, memo } );
				}else{
					draw(from, balance);
					save(to, balance);
				}
			}
		}
		
		if(internalfrom == 0 && internalto == 1){
			//external to internal
			//check whether it is mapped to external then
			//calling external to external
			pubtbl pubtable(_self, to);
			auto iter = pubtable.find(to);
			eosio_assert(iter != pubtable.end(), "to account does not exist");
			
			const auto& st = *iter;
			if(st.eos_account != N("")){
				itransfer(from, N(publytoken11), balance, memo);
				INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                            { N(publytoken11), st.eos_account, balance, memo } );
			}else{			
				itransfer(from, N(publytoken11), balance, memo);
				save(to, balance);
			}
			
		}
		
		if(internalfrom == 1 && internalto == 0){
			//internal to external
			pubtbl pubtable(_self, from);
			auto iter = pubtable.find(from);
			eosio_assert(iter != pubtable.end(), "from account does not exist");
			
			const auto& st = *iter;
			if(st.eos_account != N("")){
				itransfer(st.eos_account, N(publytoken11) , balance, memo);
				INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                            { N(publytoken11), to, balance, memo } );
			}else{
				draw(from, balance);	
				//transfer(N(publytoken11), to, balance, memo);
				INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                                { N(publytoken11), to, balance, memo } );

			}
		}
		
		if(internalfrom == 0 && internalto == 0){
			//external transfer
			itransfer(from,N(publytoken11) , balance, memo);
			INLINE_ACTION_SENDER(eosio::token, transfer)( N(publytoken11), {N(publytoken11),N(active)},
                                            { N(publytoken11), to, balance, memo } );

		}
	}
	
	void token::stake(account_name from, bool internalfrom, account_name to, bool internalto, asset quantity){
		//quantity check
		require_auth( _self );
		
		
		//existence check, from always be there
		pubtbl pubtable(_self, to);
		auto iter2 = pubtable.find(to);
		eosio_assert(iter2 != pubtable.end(), "to account does not exist");
		//if you do not set scope, then there is error, violation of constraint
		
		pubtbl pubtablefrom(_self, from);
		auto iter3 = pubtablefrom.find(from);
		eosio_assert(iter3 != pubtable.end(), "from account does not exist");
		
		staketbl3 staketbl(_self, from);
		auto iter = staketbl.find(to);
		//user duplication check
		//eosio_assert(iter == staketbl.end(), "stake from account already exists");
		//owner duplication check
		/*
		bool find_flag = 0; //initial value is false = 0
		for(auto i = staketbl.begin();i != staketbl.end(); i++){
			if(i->owner == to)
				find_flag = 1;
		}
		eosio_assert(find_flag == 0 && iter == staketbl.end(), "stake account pair already exists");
		*/
		if(iter3->eos_account != N(""))
			itransfer(iter3->eos_account, N(publytoken11), quantity, "stake event");
		else
			draw(from, quantity);
		//update stakesum table if only from != to case
		if(from != to){
			stakesum stakesumothers(_self, to);
			auto stakeiter = stakesumothers.find(quantity.symbol.name());
			if(stakeiter == stakesumothers.end()){
				stakesumothers.emplace(_self, [&]( auto& a ){
					a.balance = quantity;
				});
			}else{
				stakesumothers.modify(stakeiter, 0, [&]( auto& a ) {
					a.balance += quantity;
				});
			}
			
		}
		//update stake table
		//increase INK table by quantity - obsolsted
		//You can stake PUB to many others
		if(iter == staketbl.end()){
			staketbl.emplace( _self, [&]( auto& staketbl) {
				staketbl.balance = quantity;
				staketbl.staked_at = now();
				staketbl.user = to;
				/* delete this, because INK refill is done by server
				//increase INK power
				pubtbl pubtable(_self, to);
				auto iter2 = pubtable.find(to);
				pubtable.modify(iter2, _self, [&]( auto& pubtable ) {
					pubtable.ink = asset(quantity.amount, eosio::symbol_type(eosio::string_to_symbol(4, "INK")));	
				});
				*/
				
			});
		}else{
			staketbl.modify(iter, _self, [&]( auto& staketbl ) {
				staketbl.balance += quantity;
				staketbl.staked_at = now();
			});
		}
	}
	
	
	void token::unstake(account_name from, bool internalfrom, account_name to, bool internalto, asset quantity){
		//quantity check
		require_auth( _self );
		
		
		//existence check, from always be there
		pubtbl pubtable(_self, to);
		auto iter3 = pubtable.find(to);
		eosio_assert(iter3 != pubtable.end(), "to account does not exist");
		
		//check stake table, staketbl
		staketbl3  stake_table (_self, from);
		auto iter = stake_table.find(to);
		eosio_assert(iter != stake_table.end(), "there is no staked amount");
		
		eosio_assert(iter->balance.amount >= quantity.amount, "unstake amount overdue");
		//decrease staked amount
		stake_table.modify(iter, _self, [&]( auto& staketbl ) {
			staketbl.balance.amount -= quantity.amount;
		});
		
		//delete stake table when remaining amount is zero.
		if(iter->balance.amount == 0){
			stake_table.erase(iter);
		}
		
		//decrease stake amount from others
		if(from != to){
			stakesum stakesumothers(_self, to);
			auto stakeiter = stakesumothers.find(quantity.symbol.name());
			//eosio_assert(stakeiter == stakesumothers.end(), "there is no already staked one");
			if(stakeiter != stakesumothers.end()){
				stakesumothers.modify(stakeiter, 0, [&]( auto& a ) {
					a.balance -= quantity;
				});
			}
		}
		
		unstaketbl  unstake_table (_self, from);
		auto iter2 = unstake_table.find(to);

		if(iter2 == unstake_table.end()){
			unstake_table.emplace( _self, [&]( auto& unstaketbl) {
				unstaketbl.balance = quantity;
				unstaketbl.unstaked_at = now();
				unstaketbl.user = to;				
			});
		}else{
			unstake_table.modify(iter2, _self, [&]( auto& unstaketbl ) {
				unstaketbl.balance += quantity;
				unstaketbl.unstaked_at = now();
			});
		}
	}
	
	

void token::create( account_name issuer,
                    asset        maximum_supply )
{
    require_auth( _self );

    auto sym = maximum_supply.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( maximum_supply.is_valid(), "invalid supply");
    eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

    stat statstable( _self, sym.name() );
    auto existing = statstable.find( sym.name() );
    eosio_assert( existing == statstable.end(), "token with symbol already exists" );

    statstable.emplace( _self, [&]( auto& s ) {
       s.supply.symbol = maximum_supply.symbol;
       s.max_supply    = maximum_supply;
       s.issuer        = issuer;
    });
}


void token::issue( account_name to, asset quantity, string memo )
{
    auto sym = quantity.symbol;
    eosio_assert( sym.is_valid(), "invalid symbol name" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    auto sym_name = sym.name();
    stat statstable( _self, sym_name );
    auto existing = statstable.find( sym_name );
    eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
    const auto& st = *existing;

    require_auth( st.issuer );
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must issue positive quantity" );

    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

    statstable.modify( st, 0, [&]( auto& s ) {
       s.supply += quantity;
    });

    add_balance( st.issuer, quantity, st.issuer );

    if( to != st.issuer ) {
       SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
    }
}

void token::transfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
    eosio_assert( from != to, "cannot transfer to self" );
	
	//Prevent non-negotiated listing (S) 2018.11.27
	
	//Newdex Case
    eosio_assert( to != N(newdexpocket), "You can not transfer to Newdex in a certain period");	

	//Btex
	eosio_assert( to != N(eosbtexbonus), "You can not transfer to this exchange in a certain period");
	eosio_assert( to != N(eosconvertbt), "You can not transfer to this exchange in a certain period");
	eosio_assert( to != N(eosbtexteams), "You can not transfer to this exchange in a certain period");
	eosio_assert( to != N(btexexchange), "You can not transfer to this exchange in a certain period");
	eosio_assert( to != N(eosbtextoken), "You can not transfer to this exchange in a certain period");
	eosio_assert( to != N(eosbtexfunds), "You can not transfer to this exchange in a certain period");
	
	eosio_assert( from != N(eosbtexbonus), "You can not transfer to this exchange in a certain period");
	eosio_assert( from != N(eosconvertbt), "You can not transfer to this exchange in a certain period");
	eosio_assert( from != N(eosbtexteams), "You can not transfer to this exchange in a certain period");
	eosio_assert( from != N(btexexchange), "You can not transfer to this exchange in a certain period");
	eosio_assert( from != N(eosbtextoken), "You can not transfer to this exchange in a certain period");
	eosio_assert( from != N(eosbtexfunds), "You can not transfer to this exchange in a certain period");
	
	//Prevent non-negotiated listing (S)
	
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    
	
    auto sym = quantity.symbol.name();
    stat statstable( _self, sym );
    const auto& st = statstable.get( sym );
	
	//check whether from is locked or not in the case of PUB token
	if(quantity.symbol.name() == st.supply.symbol.name()){
		locktbl2 lockuptable( _self, _self );
		auto existing = lockuptable.find( from );
		if(existing != lockuptable.end()){
			if(existing->lockup_period == 0){
				eosio_assert( existing == lockuptable.end(), "send lockup is enabled" );
			}else{				
				asset allow_amount = asset(0, eosio::symbol_type(eosio::string_to_symbol(4, "PUB")));
				asset current_amount = get_balance(from, allow_amount.symbol.name());
				
				uint32_t t1 = existing->start_time;
				uint32_t t2 = now();
				//converting to hour
				t2 = (t2 - t1) / (60*60*24*30); //converting to milli seconds to 30days
				//t2 = (t2 - t1) / 60; //converting to milli seconds to minutes for testing
				if(t2 == 0){
					eosio_assert(false, "send lock is enable");
				}else if(t2 > existing->lockup_period){
					;//do nothing. Lock period expired
				}else{
					//lockup period is valid
					if(current_amount <= existing->initial_amount){
						allow_amount = ((existing->initial_amount * t2) / existing->lockup_period) - 
								(existing->initial_amount - current_amount);
						eosio_assert(allow_amount.amount >= quantity.amount, "send lock is enable");
					}else{
						allow_amount = ((existing->initial_amount * t2) /
								existing->lockup_period) + 
								(current_amount - existing->initial_amount);								
						eosio_assert(allow_amount.amount >= quantity.amount, "send lock is enable");
					}
				}
			}
		}
		
	}

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
	
void token::itransfer( account_name from,
                      account_name to,
                      asset        quantity,
                      string       memo )
{
	

	//require_auth( _self );
    eosio_assert( from != to, "cannot transfer to self" );
    eosio_assert( is_account( to ), "to account does not exist");
    
	
    auto sym = quantity.symbol.name();
    stat statstable( _self, sym );
    const auto& st = statstable.get( sym );
	
    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

    //auto payer = has_auth( to ) ? to : from;

    sub_balance2( from, quantity );
    add_balance2( to, quantity, from );
	//add_balance( to, quantity, _self );
}
  
void token::lock( account_name user, uint32_t period){
	
	eosio_assert( is_account( user ), "lock account does not exist");

	require_auth( _self ); //only contract owner can do this
	locktbl2 lockuptable( _self, _self );
	
	auto iter=lockuptable.find(user);
	
	if(iter == lockuptable.end()){
		//asset quantity = asset(0, eosio::symbol_type(eosio::string_to_symbol(4, "DAB")));
		symbol_type temp = eosio::symbol_type(eosio::string_to_symbol(4, "PUB"));
		asset quantity = get_balance(user, temp.name());
		lockuptable.emplace( _self, [&]( auto& lockuptable ) {
			lockuptable.user = user;
			lockuptable.initial_amount = quantity;
			lockuptable.lockup_period = period;
			lockuptable.start_time = now();
		});
	}else{
		eosio_assert(iter==lockuptable.end(), "lock account already exists in the table");
	}
}
	
void token::unlock( account_name user){
	eosio_assert( is_account( user ), "unlock account does not exist");
	require_auth( _self );
	locktbl2 lockuptable(_self, _self);
	auto itr = lockuptable.find(user);
	eosio_assert(itr != lockuptable.end(), "there is no matched unlock account in the table");
	lockuptable.erase(itr);	
}
	

void token::sub_balance2( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );

   //from_acnts.modify( from, owner, [&]( auto& a ) {
   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
	from_acnts.modify( from, _self, [&]( auto& a ) {
        a.balance -= value;
      	});
   }
}

void token::add_balance2( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      //to_acnts.emplace( ram_payer, [&]( auto& a ){
	   to_acnts.emplace( _self, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}
	
void token::sub_balance( account_name owner, asset value ) {
   accounts from_acnts( _self, owner );

   const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
   eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


   if( from.balance.amount == value.amount ) {
      from_acnts.erase( from );
   } else {
      from_acnts.modify( from, owner, [&]( auto& a ) {
          a.balance -= value;
      });
   }
}

void token::add_balance( account_name owner, asset value, account_name ram_payer )
{
   accounts to_acnts( _self, owner );
   auto to = to_acnts.find( value.symbol.name() );
   if( to == to_acnts.end() ) {
      to_acnts.emplace( ram_payer, [&]( auto& a ){
        a.balance = value;
      });
   } else {
      to_acnts.modify( to, 0, [&]( auto& a ) {
        a.balance += value;
      });
   }
}
	void token::save(account_name user, asset quantity){
		pubtbl pubtable(_self, user);
		auto iter = pubtable.find(user);
		
		eosio_assert(iter != pubtable.end(), "account does not exist");
		
		pubtable.modify(iter, _self, [&]( auto& pubtable ) {
			pubtable.balance += quantity;
		});
	}
	
	void token::draw(account_name user, asset quantity){
		pubtbl pubtable(_self, user);
		auto iter = pubtable.find(user);
		
		eosio_assert(iter != pubtable.end(), "account does not exist");
		eosio_assert( iter->balance.amount >= quantity.amount, "overdrawn balance" );
		
		
		pubtable.modify(iter, _self, [&]( auto& pubtable ) {
			pubtable.balance -= quantity;
		});
	}
	


} /// namespace eosio

EOSIO_ABI( eosio::token, (create)(issue)(transfer)(lock)(unlock)(newaccount)(check)(prepare)(pubtransfer)(stake)(unstake)(refund)(thanks)(update)(delaccount))
