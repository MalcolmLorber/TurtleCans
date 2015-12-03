/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 11-25-2015
Filename: bank.cpp
Description: Bank server that services requests from the ATM. Compile using the
             following command: g++ -std=c++11 bank.cpp -lboost_system
             -lboost_thread -lboost_regex -o bank.out 
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
#include <boost/regex.hpp>
//Program header files
#include "validate.h"
#include "bank_object.h"
#include "user.h"

using boost::asio::ip::tcp;

//Initialize a global instance of the Bank with the three users and their
//information.
Bank bank(std::vector<User> {User("alice", 100ll, 827431, 431253),
          User("bob", 50ll, 918427, 396175), User("eve", 0ll, 223175, 912343)}); 

/*******************************************************************************
 @DESC: Handle all operations relating to a session with an ATM including
        reading messages from the ATM, performing the correct action based upon
        said message, and either terminating the connection or sending a message
        back to the ATM.
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
        //Process all valid commands. A command is known to be valid before it
        //is passed to the Process function. If anything goes wrong, return
        //"error" which will logout the user and close the connection.
        std::string Process(std::string command) {
            //Handle receiving a user's pin from the ATM during the login
            //process. Log the user in if the correct information is given.
            if (login) {
                if (!boost::regex_match(command, boost::regex("^\\d{1,6}$"))) {
                    return "error";
                }
                long long result;
                try {result = stoll(command);}
                catch(std::exception & e) {return "error";}
                if (!bank.login(id, result)) {
                    return "error";
                }
                login = false;
                loggedin = true;
                return "Login successful";
            }
            //Begin the login process by recording the id of the user that is
            //trying to login and responding the ATM indicating that the user
            //must enter their pin number.
            else if (command.find("login") == 0 && !loggedin) {
                if (!login) {   
                    int space = command.find(" ") + 1;
                    try {
                        id = stoll(command.substr(space, command.size() - space));
                    }
                    catch(std::exception & e) {return "error";}
                    login = true;
                    return "Enter your pin: ";
                }
            }
            //Retrieve the current user's balance and send it to the ATM.
            else if (command.find("balance") == 0) {
                long long returnBalance = 0;
                if (!bank.balance(id, returnBalance)) {
                    return "error";
                }
                return "Balance $" + boost::lexical_cast<std::string>(returnBalance);
            }
            //Transfer money from the user currently logged in to another user.
            else if (command.find("transfer") == 0) {
                int space = command.find(" ") + 1;
                std::string temp = command.substr(space, command.size() - space);
                space = temp.find(" ") + 1;
                std::string amount = temp.substr(0, space - 1);
                std::string username = temp.substr(space, temp.size() - space);
                std::cout << "Amount: " << amount << std::endl;
                std::cout << "Username: " << username << std::endl;
                long long result;
                try {result = stoll(amount);}
                catch(std::exception & e) {return "error";}
                if (!bank.transfer(id, username, result)) {
                    return "error";
                }
                return "Transfer successful";
            }
            //Withdraw money from the current user's account.
            else if (command.find("withdraw") == 0) {
                int space = command.find(" ") + 1;
                long long amount;
                try {
                    amount = stoll(command.substr(space, command.size() - space));
                }
                catch(std::exception & e) {return "error";}
                if (!bank.withdraw(id, amount)) {
                    return "error";
                }
                return "$" + boost::lexical_cast<std::string>(amount) + 
                                                                " withdrawn";
            }
            //Logout the current user
            else if (command.find("logout") == 0) {
                if (!bank.logout(id)) {
                    return "error";
                }
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
                    //Close the connection if a message received is too large.
                    if (Length > max_length - 1) {
                        if (bank_socket_.is_open()) {
                            Process("logout");
                            bank_socket_.close();
                            return;
                        }
                    }
                    data_[Length] = '\0';
                    std::string command(data_);
                    //Close the connection if the command received is invalid.
                    if (!IsValidATMCommand(command)) {
                        if (bank_socket_.is_open()) {
                            Process("logout");
                            bank_socket_.close();
                            return;
                        }
                    }
                    std::string response = Process(command);
                    std::cout << "Before data_: " << data_ << std::endl;
                    int len = response.copy(data_, response.size(), 0);
                    data_[len] = '\0';
                    std::cout << "After data_: " << data_ << std::endl;
                    //Close the connection if an error was encountered while
                    //processing the command.
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
                    //Send the response to the ATM if the operation completed
                    //correctly.
                    DoWrite(response_length);
                }
            });
        }
        //Write to the Bank socket 
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
        long long id = 0;
        //Received user's id and waiting to receive pin.
        bool login = false;
        //User fully logged in.
        bool loggedin = false;
};

/*******************************************************************************
 @DESC: The Server class is responsible for initializing the connection to the
        ATM port and starting a Session instance.
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

//Handle the Bank's command line input
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
            //Allow the specified amount of money to be deposited to the
            //specified user.
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
            //Return the balance of a specified user.
            else if (command.find("balance") == 0){
                int space = command.find(" ") + 1;
                std::string username = command.substr(space, command.size() - space);
                long long bal = 0;
                if (!(bank.adminBalance(username, bal))){
                    std::cerr << "Unable to retrieve balance" << std::endl;
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
 @DESC: Start the Bank server and service all incoming requests with a separate
        session for each ATM connected.
 @ARGS: Port to listen on.
 @RTN:  EXIT_SUCCESS on success, EXIT_FAILURE on failure
*******************************************************************************/
int main (int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Incorrect number of arguments. Proper usage ./a.out"
                            " <bank-port>" << std::endl;
            return EXIT_FAILURE;
        }
        //Start a thread to handle command line input to the Bank.
        boost::thread Thread(CommandLine);
        boost::asio::io_service IOService;
        int BankPort = std::stoi(argv[1]);
        Server S(IOService, BankPort);
        //Throws an exception if something goes wrong
        IOService.run();
    }
    catch (std::exception & e) {
        std::cerr << "ERROR" << std::endl;
    }
    return EXIT_SUCCESS;
}
