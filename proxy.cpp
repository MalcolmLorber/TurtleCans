/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 11-10-2015
Filename: proxy.cpp
Description: Proxy program that only transports data to and from the bank
             program using the Boost libarary. Compile using the following:
             "g++ -std=c++11 proxy.cpp -lboost_system"
*******************************************************************************/

#include <iostream>
#include <boost/asio.hpp>



/*******************************************************************************
 @DESC: Listen on ATM port and connect to the bank port using the boost:asio
        library. Multiple connections must be able to be handled at the same
        time (multiple ATMs connected to the bank at any given time).
 @ARGS: port to listen on, port to connect to the bank
 @RTN:  true on success, false on failure
*******************************************************************************/
bool Connect(const int & ATMPort, const int & BankPort) {


    //If failure, return false
    

    return true;
}







/*******************************************************************************
 @DESC: Call appropriate function(s) to initiate communication between the ATM
        and the bank. 
 @ARGS: port to listen on, port to connect to the bank
 @RTN:  EXIT_SUCCESS on success, EXIT_FAILURE on failure
*******************************************************************************/
int main (int argc, char* argv[]) {

    //If Connect() returns false, return EXIT_FAILURE
    
    return EXIT_SUCCESS;
}
