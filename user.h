#ifndef _user_h_
#define _user_h_

class User{
public:
    User(std::string name, long long balance, int pin, int id): name(name), balance(balance), pin(pin), id(id){}         
    void addMoney(long long money){balance += money;}
    void removeMoney(long long money){balance -= money;}
    long long getBalance(){return balance;}
    std::string getName(){return name;}
    int getPin(){return pin;}
private:
    std::string name;
    long long balance;
    int pin;
    int id;
};

#endif 
