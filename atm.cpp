/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber                          
Date Created: 11-25-2015                                                                
Filename: proxy.cpp                                                                     
Description: Proxy program that only transports data to and from the bank               
             program using the Boost libarary. Compile using the following:             
             g++ -std=c++11 atm.cpp -lboost_system -lboost_thread -lboost_regex
             -pthread -o atm.out 
*******************************************************************************/

#include<iostream>
#include<string>
#include<regex>
#include<vector>
//#include<memory> 
//#include<utility> 
#include<boost/asio.hpp>
#include "validate.h"

using boost::asio::ip::tcp;

enum {max_length = 1024};

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
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
       
        while (true) {
            //login, balance, withdraw, transfer, logout
            std::cout << "Enter command: " ;
            std::string request;
            std::getline(std::cin, request);
            if (IsValidATMCommand(request)) {
                std::cerr << "INVALID COMMAND" << std::endl;
                return EXIT_FAILURE;
            }
            /*
	    std::vector<char> requestVec(request.begin(), request.end());
            size_t request_length = sizeof(requestVec);
            boost::asio::write(s, boost::asio::buffer(request,request_length));
	    s.send(boost::asio::buffer(requestVec));	
            */
            boost::system::error_code ignored_error;
            boost::asio::write(s, boost::asio::buffer(request),
                                boost::asio::transfer_all(), ignored_error);
            boost::asio::streambuf response;
            boost::asio::read_until(s, response, "\n");
            std::istream response_stream(&response);
            std::string answer;
            response_stream >> answer;
            std::cout << answer << std::endl;
            /*
            std::vector<char> size;
            size_t reply_length = boost::asio::read(s,
                                    boost::asio::buffer(reply, 4));
		

	    std::vector<char> reply(s.available());
            boost::asio::read(s, boost::asio::buffer(reply));

	    std::string replyStr(reply.begin(), reply.end());
            std::cout << "REPLY: " << replyStr << reply.size() << std::endl;           
            */
        }
    }
    catch (std::exception & e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
