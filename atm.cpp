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
#include<memory> //may not need
#include<utility> // may not need
#include<boost/asio.hpp>

using boost::asio::ip::tcp;


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
        
        //login, balance, withdraw, transfer, logout
        std::cout << "Enter command" ;
        // consider changing this vulnerable??
        
        char request[max_length];
        std::cin.getline(request, max_length);

        

        size_t request_length = strlen(request); //AVOID THIS
        boost::asio::write(s, boost::asio::buffer(request,request_length));

        char reply[max_length];
        size_t reply_length = boost::asio::read(s, boost::asio::buffer(reply, request_length));
        std::cout << "REPLY";
        std::cout.write(reply, reply_length);
        std::cout << '\n';           
    }
    catch (std::exception & e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
