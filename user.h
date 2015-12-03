/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, Malcolm Lorber
Date Created: 12-02-2015
Filename: user.h
*******************************************************************************/

#ifndef _user_h_
#define _user_h_

/*******************************************************************************
 @DESC: The User class contains all set() and get() type functions for a user's
        name, balance, pin, and id.
 @ARGS: N/A
 @RTN:  N/A
*******************************************************************************/
class User{
public:
    User(std::string name, long long balance, long long pin, long long id): 
                               name(name), balance(balance), pin(pin), id(id) {}         
    void addMoney(long long money) {balance += money;}
    void removeMoney(long long money) {balance -= money;}
    long long getBalance() {return balance;}
    std::string getName() {return name;}
    long long getPin() {return pin;}
    long long getID() {return id;}
private:
    std::string name;
    long long balance;
    long long pin;
    long long id;
};

#endif 
