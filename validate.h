/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 12-02-2015
Filename: validate.h
*******************************************************************************/

#ifndef _validate_h_
#define _validate_h_

//Standard Library
#include <vector>
#include <string>
//Boost
#include <boost/regex.hpp>

//Check if the given command is a valid bank command shell command
bool IsValidBankCommand(std::string command) {                                       
    std::vector<boost::regex> Allowed =
                {boost::regex("^deposit\\s{1}(alice|bob|eve)\\s{1}\\d{1,18}$"), 
                 boost::regex("^balance\\s{1}(alice|bob|eve)$")};
    for (auto &i: Allowed) {
        if (boost::regex_match(command, i)) {
            return true;
        }
    }
    return false;
} 

//Check if the given command is a valid ATM command shell command
bool IsValidATMCommand(std::string command) {                                       
    std::vector<boost::regex> Allowed =
                {boost::regex("^login\\s{1}(alice|bob|eve)$"), 
                 boost::regex("^login\\s{1}(912343|396175|431253)$"), 
                 boost::regex("^\\d{1,6}$"), 
                 boost::regex("^balance$"), 
                 boost::regex("^withdraw\\s{1}\\d{1,18}$"), 
                 boost::regex("^transfer\\s{1}\\d{1,18}\\s{1}(alice|bob|eve)$"), 
                 boost::regex("^logout$")};
    for (auto &i: Allowed) {
        if (boost::regex_match(command, i)) {
            return true;
        }
    }
    return false;
}

#endif
