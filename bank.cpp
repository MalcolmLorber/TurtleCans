/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 11-25-2015
Filename: bank.cpp
Description: Bank server that services requests from the ATM 
             using the Boost library. Compile using the following:
             g++ -std=c++11 bank.cpp -lboost_system -lboost_thread -lboost_regex
             -o bank.out 
*******************************************************************************/

//Standard library
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
//Boost
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
//Program header files
#include "validate.h"
#include "bank_object.h"
#include "user.h"

using boost::asio::ip::tcp;

Bank bank(std::vector<User> {User("alice", 100ll, 827431, 431253),
          User("bob", 50ll, 918427, 396175), User("eve", 0ll, 223175, 912343)}); 
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
        std::string Process(std::string command) {
            if (login) {
                if (command != "827431" && command != "918427"  && 
                                           command != "223175") {
                    return "error";
                }
                if (!bank.login(id, stoll(command))) {
                    return "error";
                }
                login = false;
                loggedin = true;
                return "Login successful";
            }
            else if (command.find("login") == 0 && !loggedin) {
                if (!login) {   
                    int space = command.find(" ") + 1;
                    id = stoll(command.substr(space, command.size() - space));
                    login = true;
                    return "Enter your pin: ";
                }
            }
            else if (command.find("balance") == 0) {
                long long returnBalance = 0;
                if (!bank.balance(id, returnBalance)) {
                    return "error";
                }
                return "Balance $" + boost::lexical_cast<std::string>(returnBalance);
            }
            else if (command.find("transfer") == 0) {
                int space = command.find(" ") + 1;
                std::string temp = command.substr(space, command.size() - space);
                space = temp.find(" ") + 1;
                std::string amount = temp.substr(0, space - 1);
                std::string username = temp.substr(space, temp.size() - space);
                std::cout << "Amount: " << amount << std::endl;
                std::cout << "Username: " << username << std::endl;
                if (!bank.transfer(id, username, stoll(amount))) {
                    return "error";
                }
                return "Transfer successful";
            }
            else if (command.find("withdraw") == 0) {
                int space = command.find(" ") + 1;
                long long amount = stoll(command.substr(space, command.size() - space));
                if (!bank.withdraw(id, amount)) {
                    return "error";
                }
                return "$" + boost::lexical_cast<std::string>(amount) + 
                                                                " withdrawn";
            }
            else if (command.find("logout") == 0) {
                bank.logout(id);
                loggedin = false;
                return "Logout successful";
            }
            else {
                return "error";
            }
        }
        //Read from the listen socket (ATM socket)
        void DoRead() {
            auto Self(shared_from_this());    
            bank_socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, Self](boost::system::error_code EC, std::size_t Length) {
                if ((boost::asio::error::eof == EC) ||
                        (boost::asio::error::connection_reset == EC)) {
                    Process("logout");
                }
                if (!EC) {
                    if (Length > 1023) {
                        if (bank_socket_.is_open()) {
                            bank_socket_.close();
                            return;
                        }
                    }
                    data_[Length] = '\0';
                    std::string command(data_);
                    if (!IsValidATMCommand(command)) {
                        if (bank_socket_.is_open()) {
                            bank_socket_.close();
                            return;
                        }
                    }
                    std::string response = Process(command);
                    std::cout << "Before data_: " << data_ << std::endl;
                    int len = response.copy(data_, response.size(), 0);
                    data_[len] = '\0';
                    std::cout << "After data_: " << data_ << std::endl;
                    if (response == "error") {
                        if (bank_socket_.is_open()) {
                            //Logout before closing the socket
                            Process("logout");
                            bank_socket_.close();
                            return;
                        }
                    }
                    int response_length = response.size();
		    std::cout << "Message received: " << command << std::endl;
                    std::cout << "Returned response: " << response << std::endl;
                    DoWrite(response_length);
                }
            });
        }
        //Write to the bank socket 
        void DoWrite(std::size_t Length) {
            std::cout << data_ << std::endl;
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
        long long id;
        bool login = false;
        bool loggedin = false;
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
    std::string command;
    while (true){
	if (!std::getline(std::cin, command)){
	    std::cin.clear();
	    continue;
	}
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
