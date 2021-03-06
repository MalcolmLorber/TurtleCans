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
#include <algorithm>
//Boost
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
//Program header files
#include "validate.h"
#include "bank_object.h"
#include "user.h"
//Crypto++
#include "cryptopp/osrng.h"
#include "cryptopp/integer.h"
#include "cryptopp/nbtheory.h"
#include "cryptopp/dh.h"
#include "cryptopp/secblock.h"
#include "cryptopp/hex.h"
#include "cryptopp/filters.h"
#include "cryptopp/sha.h"
#include "cryptopp/aes.h"
#include "cryptopp/modes.h"
#include "cryptopp/rsa.h"
#include "cryptopp/base64.h"
#include "cryptopp/files.h"

using boost::asio::ip::tcp;

using namespace CryptoPP;
using namespace std;

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
            : bank_socket_(std::move(Socket1)) {
            setupenc();
        }
        ~Session(){
            delete cfbEncryption;
            delete cfbDecryption;
        }
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
                try {
                    id = stoll(command.substr(space, command.size() - space));
                }
                catch(std::exception & e) {return "error";}
                login = true;
                return "Enter your pin: ";
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
                std::string data64dec;
                std::string sdata(data_);
                StringSource ss(sdata,true, new Base64Decoder(new StringSink(data64dec)));
                std::string decryptedRequest; 
                StringSource ds(data64dec, true, new StreamTransformationFilter(*cfbDecryption,new StringSink(decryptedRequest)));
                
                strcpy(data_, decryptedRequest.c_str());
                if(decryptedRequest.size()<max_length){
                    data_[decryptedRequest.size()]='\0';
                }
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
                    int len = response.copy(data_, response.size(), 0);
                    data_[len] = '\0';
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
                    //Send the response to the ATM if the operation completed
                    //correctly.
                    DoWrite(response_length);
                }
                memset(&data_[0],0,sizeof(data_));
            });
        }
        //Write to the Bank socket 
        void DoWrite(std::size_t Length) {
            auto Self(shared_from_this());
            
            std::string request(data_);
            std::string encryptedRequest;
            StringSource es(request, true, new StreamTransformationFilter(*cfbEncryption,new StringSink(encryptedRequest)));
            std::string encryptedRequest64;
            StringSource aesEncode(encryptedRequest,true,new Base64Encoder(new StringSink(encryptedRequest64)));
            encryptedRequest64.erase(std::remove(encryptedRequest64.begin(),encryptedRequest64.end(),'\n'), encryptedRequest64.end());
            strcpy(data_, encryptedRequest64.c_str());
            Length = encryptedRequest64.size();
            boost::asio::async_write(bank_socket_, 
                                    boost::asio::buffer(data_, Length),
                        [this, Self](boost::system::error_code EC, std::size_t){
                if (!EC) {
                    DoRead();
                }
            });
        }
    
    CryptoPP::CFB_Mode<AES>::Encryption* cfbEncryption;
    CryptoPP::CFB_Mode<AES>::Decryption* cfbDecryption;
    void setupenc(){
        CryptoPP::Integer p("0x87A8E61DB4B6663CFFBBD19C651959998CEEF608660DD0F2\
                          5D2CEED4435E3B00E00DF8F1D61957D4FAF7DF4561B2AA30\
                          16C3D91134096FAA3BF4296D830E9A7C209E0C6497517ABD\
                          5A8A9D306BCF67ED91F9E6725B4758C022E0B1EF4275BF7B\
                          6C5BFC11D45F9088B941F54EB1E59BB8BC39A0BF12307F5C\
                          4FDB70C581B23F76B63ACAE1CAA6B7902D52526735488A0E\
                          F13C6D9A51BFA4AB3AD8347796524D8EF6A167B5A41825D9\
                          67E144E5140564251CCACB83E6B486F6B3CA3F7971506026\
                          C0B857F689962856DED4010ABD0BE621C3A3960A54E710C3\
                          75F26375D7014103A4B54330C198AF126116D2276E11715F\
                          693877FAD7EF09CADB094AE91E1A1597");
        CryptoPP::Integer g("0x3FB32C9B73134D0B2E77506660EDBD484CA7B18F21EF2054\
                          07F4793A1A0BA12510DBC15077BE463FFF4FED4AAC0BB555\
                          BE3A6C1B0C6B47B1BC3773BF7E8C6F62901228F8C28CBB18\
                          A55AE31341000A650196F931C77A57F2DDF463E5E9EC144B\
                          777DE62AAAB8A8628AC376D282D6ED3864E67982428EBC83\
                          1D14348F6F2F9193B5045AF2767164E1DFC967C1FB3F2E55\
                          A4BD1BFFE83B9C80D052B985D182EA0ADB2A3B7313D3FE14\
                          C8484B1E052588B9B7D2BBD2DF016199ECD06E1557CD0915\
                          B3353BBB64E0EC377FD028370DF92B52C7891428CDC67EB6\
                          184B523D1DB246C32F63078490F00EF8D647D148D4795451\
                          5E2327CFEF98C582664B4C0F6CC41659");
        CryptoPP::Integer q("0x8CF83642A709A097B447997640129DA299B1A47D1EB3750BA308B0FE64F5FBD3");
        
        AutoSeededRandomPool rndB;
        boost::system::error_code EC;
        
        FileSource privfile("privkey.txt", true, new Base64Decoder);
        FileSource pubfile("pubkey.txt", true, new Base64Decoder);
        FileSource atmpubfile("atmpubkey.txt",true,new Base64Decoder);
        ByteQueue privbytes;
        privfile.TransferTo(privbytes);
        RSA::PrivateKey privkey;
        privkey.Load(privbytes);
        
        ByteQueue pubbytes;
        pubfile.TransferTo(pubbytes);
        RSA::PublicKey pubkey;
        pubkey.Load(pubbytes);
        
        ByteQueue atmpubbytes;
        atmpubfile.TransferTo(atmpubbytes);
        RSA::PublicKey atmpubkey;
        atmpubkey.Load(atmpubbytes);

        boost::asio::streambuf pubresponse;
        boost::asio::read_until(bank_socket_, pubresponse, "\0");
        std::istream pub_response_stream(&pubresponse);
        std::string pubanswer;
        std::getline(pub_response_stream, pubanswer);
        if (pubanswer.size() < 340) {
            throw runtime_error("ERROR");
        }
        
        std::string pubdec;
        StringSource pubrss(pubanswer, true, new Base64Decoder(new StringSink(pubdec)));

        Integer c((const byte*)pubdec.data(),pubdec.size());
        Integer r = privkey.CalculateInverse(rndB, c);
        std::string chalrecovered;
        size_t req = r.MinEncodedSize();
        chalrecovered.resize(req);
        r.Encode((byte *)chalrecovered.data(), chalrecovered.size());
        std::string chaldisp;
        StringSource(chalrecovered, true,
		new HexEncoder( new StringSink(chaldisp)) ); 
        Integer m ((const byte*)chalrecovered.data(),128);
        Integer c2 = atmpubkey.ApplyFunction(m);
        
        std::string challengeenc;
        size_t req2 = c2.MinEncodedSize();
        challengeenc.resize(req2);
        c2.Encode((byte*)challengeenc.data(),challengeenc.size());
        
        std::string challenge64;
        StringSource pubss(challengeenc, true, new Base64Encoder(new StringSink(challenge64)));
        challenge64.erase(std::remove(challenge64.begin(),challenge64.end(),'\n'),challenge64.end());
        boost::asio::write(bank_socket_, boost::asio::buffer(challenge64),
                                boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }
        
        DH dhB;

        dhB.AccessGroupParameters().Initialize(p, q, g);

        if(!dhB.GetGroupParameters().ValidateGroup(rndB, 3)){
	    throw runtime_error("Failed to validate prime and generator");
	}

        size_t count = 0;

        Integer v = ModularExponentiation(g, q, p);
        if(v != Integer::One()){
	   throw runtime_error("Failed to verify order of the subgroup");
	}

        SecByteBlock privB(dhB.PrivateKeyLength());
        SecByteBlock pubB(dhB.PublicKeyLength());
        dhB.GenerateKeyPair(rndB, privB, pubB);

        //EXCHANGE PUBLICS HERE
        //recv(pubA)
        boost::asio::streambuf response;
        boost::asio::read_until(bank_socket_, response, "\0");
        std::istream response_stream(&response);
        std::string answer;
        std::getline(response_stream, answer);
        //base64decode it
        std::string pubAstr;
        StringSource ss2(answer, true, new Base64Decoder(new StringSink(pubAstr)));
        SecByteBlock pubA((byte*)pubAstr.data(),pubAstr.size());
        //send(pubB)
        std::string pubB64;
        StringSource ss(pubB.data(),pubB.size()+1,true,new Base64Encoder(new StringSink(pubB64)));
        pubB64.erase(std::remove(pubB64.begin(),pubB64.end(),'\n'),pubB64.end());
        
        boost::asio::write(bank_socket_, boost::asio::buffer(pubB64),
                                boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }

        SecByteBlock sharedB(dhB.AgreedValueLength());
        
        if(!dhB.Agree(sharedB, privB, pubA)){
	    throw runtime_error("Failed to reach shared secret (B)");	
	}
        	    
        Integer b;
	b.Decode(sharedB.BytePtr(), sharedB.SizeInBytes());

        int aesKeyLength = SHA256::DIGESTSIZE;
        int defBlockSize = AES::BLOCKSIZE;
        SecByteBlock key(SHA256::DIGESTSIZE);
        SHA256().CalculateDigest(key, sharedB, sharedB.size());
        byte iv1[AES::BLOCKSIZE];
        rndB.GenerateBlock(iv1, AES::BLOCKSIZE);

        //Capture the ack from the ATM
        boost::asio::streambuf ack_response;
        boost::asio::read_until(bank_socket_, ack_response, "\0");
        std::istream ack_stream(&ack_response);
        std::string ack;
        std::getline(ack_stream, ack);

        std::string iv164, iv264;
        StringSource ssiv1(iv1,17,true,new Base64Encoder(new StringSink(iv164)));
        iv164.erase(std::remove(iv164.begin(),iv164.end(),'\n'),iv164.end());
        iv164.append("\0");
        boost::asio::write(bank_socket_, boost::asio::buffer(iv164),
                           boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }
        cfbEncryption = new CryptoPP::CFB_Mode<AES>::Encryption(key, aesKeyLength, iv1);
        cfbDecryption = new CryptoPP::CFB_Mode<AES>::Decryption(key, aesKeyLength, iv1);
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
			try{
                        	std::make_shared<Session>(std::move(bank_socket_)) -> Start();
			}
			catch(std::exception e){
			  std::cout << "Handshake Failed" <<std::endl;                        
			}
                        DoAccept();
		}	
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
