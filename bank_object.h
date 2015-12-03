/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 12-02-2015
Filename: bank_object.h
*******************************************************************************/

#ifndef _bank_h_
#define _bank_h_

//Standard library
#include <vector>
#include "limits.h"
//Program header files
#include "user.h"

/*******************************************************************************
 @DESC: The Bank class implements all required functions for interacting with
        the Bank, both over TCP and using the Bank's command line interface.
 @ARGS: N/A
 @RTN : N/A
*******************************************************************************/
class Bank{
public:
    Bank(std::vector<User> uVec){
      for (unsigned int i =0; i< uVec.size(); ++i){
        std::string uName = uVec[i].getName();
        long long id = uVec[i].getID();
        loggedin[uName]=false;
        users[uName] = new User(uVec[i]);       
        ids[id] = new User(uVec[i]);       
      }
    }
    bool login(long long id, long long pin){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(ids.count(id)!=1){
            data_m.unlock();
            return false;
        }
        //Do not allow a user to login if already logged in.
        if(loggedin[ids[id]->getName()]){
            data_m.unlock();
            return false;
        }
        //Check for an invalid pin
        if(ids[id]->getPin() != pin){
            data_m.unlock();
            return false;
        }
        loggedin[ids[id]->getName()]=true;
        data_m.unlock();
        return true;
    }
    bool balance(long long id, long long &retBal){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(ids.count(id)!=1){
            data_m.unlock();
            return false;
        }
        //Ensure that a user is logged in
        if(!loggedin[ids[id]->getName()]){
            data_m.unlock();
            return false;
        }
        retBal = users[ids[id]->getName()]->getBalance();
        data_m.unlock();
        return true;
    }
    bool adminBalance(std::string user, long long &retBal){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(users.count(user)!=1){
            data_m.unlock();
            return false;
        }
        retBal = users[user]->getBalance();
        data_m.unlock();
        return true;
    }
    bool deposit(std::string user, long long amount){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(users.count(user)!=1){
            data_m.unlock();
            return false;
        }
        long long userBal = users[user]->getBalance();
        //Ensure that a long long can hold the resulting balance
        if (amount > LLONG_MAX - userBal){
            data_m.unlock();
            return false;    
        }
        users[user]->addMoney(amount);
        data_m.unlock();
        return true; 
    }
    bool withdraw(long long id, long long amount){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(ids.count(id)!=1){
            data_m.unlock();
            return false;
        }
        //Ensure the user is logged in.
        if(!loggedin[ids[id]->getName()]){
            data_m.unlock();
            return false;
        }
        //Do not allow withdraws of 0 or less dollars
        if(amount<=0){
            data_m.unlock();
            return false;
        }
        //Ensure the user has sufficient funds for the withdraw
        if(users[ids[id]->getName()]->getBalance()<amount){
            data_m.unlock();
            return false;
        }
        users[ids[id]->getName()]->removeMoney(amount);
        data_m.unlock();
        return true;
    }
    bool transfer(long long id, std::string user2, long long amount){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(ids.count(id)!=1){
            data_m.unlock();
            return false;
        }
        //Ensure the user is logged in.
        if(!loggedin[ids[id]->getName()]){
            data_m.unlock();
            return false;
        }
        if(users[ids[id]->getName()]->getBalance()<amount){
            data_m.unlock();
            return false;
        }
        //Do not allow transfers of 0 or less dollars
        if(amount<=0){
            data_m.unlock();
            return false;
        }
        //Ensure that the id exists in the unordered map
        if(users.count(user2)!=1){
            data_m.unlock();
            return false;
        }
        long long user2Bal=users[user2]->getBalance();
        //Ensure the transfer will not cause a long long to overflow
        if (amount > LLONG_MAX - user2Bal){
            data_m.unlock();
            return false;    
        }
        users[ids[id]->getName()]->removeMoney(amount);        
        users[user2]->addMoney(amount);
        data_m.unlock();
        return true;
    }
    bool logout(long long id){
        data_m.lock();
        //Ensure that the id exists in the unordered map
        if(ids.count(id)!=1){
            data_m.unlock();
            return false;
        }
        //Ensure the user is logged in.
        if(!loggedin[ids[id]->getName()]){
            data_m.unlock();
            return false;
        }
        loggedin[ids[id]->getName()]=false;
        data_m.unlock();
        return true;
    }
private:
    std::unordered_map<std::string, bool> loggedin;
    std::unordered_map<std::string, User*> users;
    std::unordered_map<long long, User*> ids;
    std::mutex data_m;
};

#endif 
