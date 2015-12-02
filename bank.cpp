/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 11-25-2015
Filename: bank.cpp
Description: Bank server that services requests from the ATM 
             using the Boost library. Compile using the following:
             g++ -std=c++11 bank.cpp -lboost_system -lboost_thread -lboost_regex
             -o bank.out 
*******************************************************************************/

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include "validate.h"
#include <mutex>
#include "limits.h"
using boost::asio::ip::tcp;

class User{
public:
    User(std::string name, long long balance, int pin): name(name), balance(balance), pin(pin){}         
    void addMoney(long long money){balance += money;}
    void removeMoney(long long money){balance -= money;}
    long long getBalance(){return balance;}
    std::string getName(){return name;}
    int getPin(){return pin;}
private:
    std::string name;
    long long balance;
    int pin;
};

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

Bank bank(std::vector<User> {User("alice", 100ll, 827431), User("bob", 50ll, 91842), User("eve", 0ll, 22317)}); 
/*******************************************************************************
 @DESC: The Session class is responsible for reading from the ATM socket and
        to the Bank socket.
 @ARGS: N/A
 @RTN:  N/A
*******************************************************************************/
class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket Socket1) 
            : bank_socket_(std::move(Socket1)) {}
        void Start() {
            DoRead();
        }
    private:
        //Read from the listen socket (ATM socket)
        void DoRead() {
            auto Self(shared_from_this());    
            bank_socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, Self](boost::system::error_code EC, std::size_t Length) {
                if (!EC) {
                    data_[Length] = '\0';
		    std::cout << "Message received: " << data_ << std::endl;
                    //instead of directly writing, perform the correct operation
                    //if(std::string(data_).find("login") == 0){
                    //    sprintf(data_, "successful write to socket");
                    //}            
                    DoWrite(Length);
                }
            });
        }
        //Write to the bank socket 
        void DoWrite(std::size_t Length) {
            auto Self(shared_from_this());
            boost::asio::async_write(bank_socket_, 
                                    boost::asio::buffer(data_, Length),
                        [this, Self](boost::system::error_code EC, std::size_t){
                if (!EC) {
                    DoRead();
                }
            });
        }
        tcp::socket bank_socket_;
        //Max length of messages passed through the proxy
        enum {max_length = 1024};
        //Array to hold the incoming message
        char data_[max_length];
        //string data_;
};

/*******************************************************************************
 @DESC: The Server class is responsible for initializing the connection to both
        ATM and Bank ports and start a Session instance.
 @ARGS: N/A
 @RTN:  N/A
*******************************************************************************/
class Server {
    public:
        Server(boost::asio::io_service & IOService, int BankPort)
            //Call acceptor constructor with the bank's port as the endopoint 
            //and assign result to acceptor_
            : bank_acceptor_(IOService, tcp::endpoint(tcp::v4(), BankPort)),
              bank_socket_(IOService) {
                DoAccept();
              }
    private:
        void DoAccept() {
            //Accept a new connection to the bank socket
            bank_acceptor_.async_accept(bank_socket_,
                                         [this](boost::system::error_code EC) {
                if (!EC) {
                        //Accept connection and start a session by calling the
                        //Session constructor
                        std::make_shared<Session>(std::move(bank_socket_)) -> Start();
                        }
                        DoAccept();
                    });
        }
        //Bank acceptor and socket
        tcp::acceptor bank_acceptor_;
        tcp::socket bank_socket_;
};

void CommandLine() {
    //p_o = segment.find<Bank>("MyInstance").first; 
    while (true) {
        std::string command;
        std::getline(std::cin, command);
        bool matched = IsValidBankCommand(command);
        if (!matched) {
            std::cerr << "INVALID COMMAND" << std::endl;
        }
        else {
            if(command.find("deposit") == 0) {
                int space1 = command.find(" ") +1;
                std::string token1 = command.substr(space1, command.size() - space1);
                int space2 = token1.find(" ");
                std::string username = token1.substr(0, space2);
                long long amount = 0;
               
                try{
                    amount = stoll(token1.substr(space2+1, token1.size()-space2-1));               
                }
                catch(std::exception & e){
                    std::cerr << "Invalid deposit amount" << std::endl;
                    continue; 
                }
               if (!(bank.deposit(username, amount))){
                    std::cerr << "Unable to deposit money" << std::endl;
                    continue;
                }    
                else{
                    std::cout << "Balance increased by $" << amount << std::endl;
                } 
            }
            else if (command.find("balance") == 0){
                int space = command.find(" ") + 1;
                std::string username = command.substr(space, command.size() - space);
                long long bal = 0;
                if (!(bank.adminBalance(username, bal))){
                    std::cerr << "Unable to retreive balance" << std::endl;
                    continue;
                }    
                else{
                    std::cout << "Balance: $" << bal << std::endl;
                }
            }
        }
    }
}


/*******************************************************************************
 @DESC: Call appropriate function(s) to initiate communication between the ATM
        and the bank. 
 @ARGS: port to listen on, port to connect to the bank
 @RTN:  EXIT_SUCCESS on success, EXIT_FAILURE on failure
*******************************************************************************/
int main (int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Incorrect number of arguments. Proper usage ./a.out"
                            " <bank-port>" << std::endl;
            return EXIT_FAILURE;
        }
        boost::thread Thread(CommandLine);
        boost::asio::io_service IOService;
        int BankPort = std::stoi(argv[1]);
        Server S(IOService, BankPort);
        //Throws an exception if something goes wrong
        IOService.run();
    }
    catch (std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
