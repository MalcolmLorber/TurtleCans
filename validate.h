#include <boost/regex.hpp>
#include <vector>
#include <string>

//Check if the given command is a valid bank command shell command
bool IsValidBankCommand(std::string command) {                                       
    std::vector<boost::regex> Allowed =
                    {boost::regex("^deposit\\s{1}(alice|bob|eve)\\s{1}\\d+$"), 
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
                 boost::regex("^balance$"), 
                 boost::regex("^withdraw\\s{1}\\d{1,20}$"), 
                 boost::regex("^transfer\\s{1}\\d{1,20}\\s{1}(alice|bob|eve)$"), 
                 boost::regex("^logout$")};
    for (auto &i: Allowed) {
        if (boost::regex_match(command, i)) {
            return true;
        }
    }
    return false;
}