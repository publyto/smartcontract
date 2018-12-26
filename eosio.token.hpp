/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   class token : public contract {
      public:
         token( account_name self ):contract(self){}

         void create( account_name issuer,
                      asset        maximum_supply);

         void issue( account_name to, asset quantity, string memo );

         void transfer( account_name from,
                        account_name to,
                        asset        quantity,
                        string       memo );

         //@abi action
         void lock(account_name user, uint32_t period);
         //@abi action
         void unlock(account_name user);
	   
         //@abi action
         void check(account_name euser, account_name iuser, string memo);
	   
         //@abi action
         void prepare(account_name euser, account_name iuser, string memo);
	   
	 //@abi action
	 void newaccount(account_name iuser);
	   
	 //@abi action
	 void delaccount(account_name euser);

	     //@abi action
         void stake(account_name from, bool internalfrom, account_name to, bool internalto, asset quantity);
	     //@abi action
         void unstake(account_name from, bool internalfrom, account_name to, bool internalto, asset quantity);
	     //@abi action
         void update(account_name user, asset quantity);
	 	 //@abi action
         void pubtransfer(account_name from, bool internalfrom, account_name to, bool internalto, asset balance, string memo);
	   
	     void refund(account_name from, account_name user);
	     void thanks(account_name user, asset quantity, string boardid);

         inline asset get_supply( symbol_name sym )const;
         
         inline asset get_balance( account_name owner, symbol_name sym )const;
	   


      private:
         //@abi table accounts i64
         struct account {
            asset    balance;
            uint64_t primary_key()const { return balance.symbol.name(); }
         };
         //@abi table stat i64
         struct currency_stat {
            asset          supply;
            asset          max_supply;
            account_name   issuer;

            uint64_t primary_key()const { return supply.symbol.name(); }
         };
	   
         //@abi table stakesum i64
         struct stakesum_table {
            asset    balance;
            uint64_t primary_key()const { return balance.symbol.name(); }
         };
	   
	     //@abi table contbl2 i64
         struct conn_table {
	    account_name user;	 
            uint64_t    status;
            uint64_t primary_key()const { return user; }
         };
	   
	 //@abi table maptbl i64
	 struct map_table {
		 account_name euser;
		 account_name iuser;
		 
		 uint64_t primary_key()const {return euser;}
		 EOSLIB_SERIALIZE(map_table,(euser)(iuser))
	 };
	
      
         //@abi table locktbl2 i64
         struct lockup_list {
            account_name user;
            asset initial_amount;
            uint32_t lockup_period;
            uint32_t start_time;
            
            uint64_t primary_key()const {return user;}
            EOSLIB_SERIALIZE(lockup_list,(user)(initial_amount)(lockup_period)(start_time))
         };
      
         //@abi table pubtbl i64
         struct pub_table {
            account_name user;
            account_name eos_account;
            asset balance;
	    	asset ink;	         
            
            uint64_t primary_key()const {return user;}
            EOSLIB_SERIALIZE(pub_table,(user)(eos_account)(balance)(ink))
         };
	      //@abi table staketbl3 i64
	      struct stake_table {
			account_name user;
			asset balance;
			uint32_t staked_at;
		   
		uint64_t primary_key()const {return user;}
		EOSLIB_SERIALIZE(stake_table,(user)(balance)(staked_at))
	      }; 
	   
	      //@abi table unstaketbl i64
	      struct unstake_table {
		    account_name user;
		    asset balance;
		    uint32_t unstaked_at;
		   
		    uint64_t primary_key()const {return user;}
		    EOSLIB_SERIALIZE(unstake_table,(user)(balance)(unstaked_at))
	      }; 
            

         typedef eosio::multi_index<N(accounts), account> accounts;
	 	 typedef eosio::multi_index<N(stakesum), stakesum_table> stakesum;
	     typedef eosio::multi_index<N(contbl2), conn_table> contbl2;
         typedef eosio::multi_index<N(stat), currency_stat> stat;
         typedef eosio::multi_index<N(locktbl2), lockup_list> locktbl2;
      
         typedef eosio::multi_index<N(pubtbl), pub_table> pubtbl;
		 typedef eosio::multi_index<N(staketbl3), stake_table> staketbl3;
	 	 typedef eosio::multi_index<N(unstaketbl), unstake_table> unstaketbl;
	 	 typedef eosio::multi_index<N(maptbl), map_table> maptbl;

         void sub_balance( account_name owner, asset value );
         void add_balance( account_name owner, asset value, account_name ram_payer );
	   
	 void sub_balance2( account_name owner, asset value );
         void add_balance2( account_name owner, asset value, account_name ram_payer );
	   

         void save(account_name user, asset quantity);

         void draw(account_name user, asset quantity);

	 	 void itransfer( account_name from,
                         account_name to,
                         asset        quantity,
                         string       memo );


      public:
         struct transfer_args {
            account_name  from;
            account_name  to;
            asset         quantity;
            string        memo;
         };
   };

   asset token::get_supply( symbol_name sym )const
   {
      stat statstable( _self, sym );
      const auto& st = statstable.get( sym );
      return st.supply;
   }

   asset token::get_balance( account_name owner, symbol_name sym )const
   {
      accounts accountstable( _self, owner );
      const auto& ac = accountstable.get( sym );
      return ac.balance;
   }

} /// namespace eosio
