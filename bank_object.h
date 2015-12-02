#ifndef _bank_h_
#define _bank_h_

#include <vector>
#include "limits.h"
#include "user.h"

class Bank{
public:
    Bank(std::vector<User> uVec){
      //userVec = uVec;
      for (unsigned int i =0; i< uVec.size(); ++i){
        std::string uName = uVec[i].getName();
        loggedin[uName]=false;
        users[uName] = new User(uVec[i]);       
      }
    }
    bool login(std::string user, int pin){
        data_m.lock();

        if(users.count(user)!=1){
            data_m.unlock();
            return false;
        }
        if(users[user]->getPin() != pin){
            data_m.unlock();
            return false;
        }
        
        loggedin[user]=true;

        data_m.unlock();
        return true;
    }
    bool balance(std::string user, long long &retBal){
        data_m.lock();

        if(!loggedin[user]){
            data_m.unlock();
            return false;
        }
        retBal = users[user]->getBalance();

        data_m.unlock();
        return true;
    }
    bool adminBalance(std::string user, long long &retBal){
        data_m.lock();
        retBal = users[user]->getBalance();
        data_m.unlock();
        return true;
    }
    bool deposit(std::string user, long long amount){
        data_m.lock();
        
        long long userBal = users[user]->getBalance();
        
        if (amount > LLONG_MAX - userBal){
            data_m.unlock();
            return false;    
        }
        
        users[user]->addMoney(amount);
        
        data_m.unlock();
        return true; 
    }

    bool withdraw(std::string user, long long amount){
        data_m.lock();

        if(!loggedin[user]){
            data_m.unlock();
            return false;
        }
        if(amount<=0){
            data_m.unlock();
            return false;
        }
        if(users[user]->getBalance()<amount){
            data_m.unlock();
            return false;
        }
        users[user]->removeMoney(amount);

        data_m.unlock();
        return true;
    }
    bool transfer(std::string user1, std::string user2, long long amount){
        data_m.lock();

        if(!loggedin[user1]){
            data_m.unlock();
            return false;
        }
        if(users[user1]->getBalance()<amount){
            data_m.unlock();
            return false;
        }
        if(amount<=0){
            data_m.unlock();
            return false;
        }
        if(users.count(user2)!=1){
            data_m.unlock();
            return false;
        }
        long long user2Bal=users[user2]->getBalance();
        
        if (amount > LLONG_MAX - user2Bal){
            data_m.unlock();
            return false;    
        }
        users[user1]->removeMoney(amount);        
        users[user2]->addMoney(amount);

        data_m.unlock();
        return true;
    }
    bool logout(std::string user){
        data_m.lock();
        
        if(!loggedin[user]){
            data_m.unlock();
            return false;
        }
        
        loggedin[user]=false;
        
        data_m.unlock();
        return true;
    }
private:
    std::unordered_map<std::string, bool> loggedin;
    std::unordered_map<std::string, User*> users;
    std::mutex data_m;
};

#endif 
