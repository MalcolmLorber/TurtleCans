/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber                          
Date Created: 11-25-2015                                                                
Filename: proxy.cpp                                                                     
Description: Proxy program that only transports data to and from the bank               
             program using the Boost libarary. Compile using the following:             
             "g++ -std=c++11 proxy.cpp -lboost_system -pthread"                                  
*******************************************************************************/

#include<iostream>
#include<string>
#include<regex>
#include<vector>
#include<memory> //may not need
#include<utility> // may not need
#include<boost/asio.hpp>

using boost::asio::ip::tcp;


//these should essentially all do the same thing
//may not need multiple functions
void login(){
}
void balance(){}
void withdraw(){}
void transfer(){}
void logout(){
    std::cout << "Session terminated" << std::endl; 
}

void ProcessCommand(std::string command){
    // TODO: consider setting limit to username
    // Use regex to check for valid command
    std::vector<std::regex> ValidCommands = {std::regex("^login\\[.+\\]$"), std::regex("^balance$"), std::regex("^withdraw\\[.+\\]$"), std::regex("^transfer\\[.+\\]\\[.+\\]$"), std::regex("^logout$")}; 
    bool matched = false;

    for (auto &i: ValidCommands){
        if (std::regex_match(command,i)){
            matched=true;
            break;
        }
    }
    if (!matched){
        std::cerr << "INVALID COMMAND" << std::endl;
        return;
    }
    if(command.find("login") == 0)          login(); 
    else if (command == "balance")          balance();
    else if(command.find("withdraw") == 0)  withdraw();
    else if (command.find("transfer") == 0) transfer();
    else if (command == "logout")           logout();
}

enum {max_length = 1024};

int main(int argc, char** argv){
    try{
        if (argc != 2){
            std::cerr << "ERROR: USAGE <port-to-proxy>" << std::endl;
            return EXIT_FAILURE;
        }
        const std::string ProxyPort = argv[1]; 
        const std::string host = "127.0.0.1";
        boost::asio::io_service io_service;
    
        tcp::resolver resolver(io_service);
        tcp::resolver::query query(tcp::v4(), host, ProxyPort);
        tcp::resolver::iterator iterator = resolver.resolve(query);
        
        tcp::socket s(io_service);
        boost::asio::connect(s, iterator);
       
        while (true){
            //login, balance, withdraw, transfer, logout
            std::cout << "Enter command: " ;
            // consider changing this vulnerable??
            
            std::string request;
            std::getline(std::cin, request);
            //char request[max_length];
            //std::cin.getline(request, max_length);
            
            ProcessCommand(request);

	    std::vector<char> requestVec(request.begin(), request.end());
            
            size_t request_length = requestVec.size();
            //boost::asio::write(s, boost::asio::buffer(request,request_length));
	    s.send(boost::asio::buffer(requestVec));	
            std::vector<char> reply;
            //size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply, request_length));
	    //s.receive(boost::asio::buffer(reply));
		

	    std::vector<char> data(s.available());
            boost::asio::read(s, boost::asio::buffer(data));

	    std::string replyStr(reply.begin(), reply.end());
            std::cout << "REPLY: " << replyStr << reply.size() << std::endl;           
        }
    }
    catch (std::exception & e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
