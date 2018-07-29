/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#include <eosiolib/eosio.hpp>


using namespace eosio;

class wafyartvotes : public eosio::contract {
      public:
        using contract::contract;

        // @abi table  
        // @abi action alltickets i64
        struct allticket{
            int64_t id;
            int64_t total;
            int64_t balance;
            int64_t locking;

            uint64_t primary_key()const {return id};
        }
        void hi( account_name user ) {
         print( "Hello, ", name{user} );
        }
        get_action
};

