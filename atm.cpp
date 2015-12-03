/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber                          
Date Created: 11-25-2015                                                                
Filename: proxy.cpp                                                                     
Description: ATM program that is used to communicate with the Bank securely (see
             README for more information). Compile using the following command:             
             g++ -std=c++11 atm.cpp -lboost_system -lboost_thread -lboost_regex
             -pthread -o atm.out 
*******************************************************************************/

//Standard library
#include <iostream>
#include <string>
#include <fstream>
#include <algorithm>
#include <iomanip>
//Boost
#include <boost/asio.hpp>
//Program header files
#include "validate.h"
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

/*******************************************************************************
 @DESC: Establish connection to the Bank, send the user inputted commands to the
        Bank (if valid), receive the response from the Bank, and display the
        response to the user. The user is able to continually enter commands,
        even after entering an invalid command.
 @ARGS: Port number used to connect to the proxy.
 @RTN:  EXIT_SUCCESS on success, EXIT_FAILURE on failure.
*******************************************************************************/
int main(int argc, char** argv) {
    try {
        
        Integer p("0x87A8E61DB4B6663CFFBBD19C651959998CEEF608660DD0F2\
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
        Integer g("0x3FB32C9B73134D0B2E77506660EDBD484CA7B18F21EF2054\
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
        Integer q("0x8CF83642A709A097B447997640129DA299B1A47D1EB3750BA308B0FE64F5FBD3");
        
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
        std::string request;

        //Handshake begins here
        //ATM RSA private key
        FileSource privfile("atmprivkey.txt", true, new Base64Decoder);
        //ATM RSA public key
        FileSource pubfile("atmpubkey.txt", true, new Base64Decoder);
        //Bank RSA public key
        FileSource bankpubfile("pubkey.txt", true, new Base64Decoder);

        //Load ATM RSA private key taken from the file into a RSA private key 
        //primitive (i.e. make it usable to the code that follows).
        ByteQueue privbytes;
        privfile.TransferTo(privbytes);
        RSA::PrivateKey privkey;
        privkey.Load(privbytes);

        //Load ATM RSA public key taken from the file into a RSA private key 
        //primitive (i.e. make it usable to the code that follows).
        ByteQueue pubbytes;
        pubfile.TransferTo(pubbytes);
        RSA::PublicKey pubkey;
        pubkey.Load(pubbytes);

        //Load Bank RSA public key taken from the file into a RSA private key 
        //primitive (i.e. make it usable to the code that follows).
        ByteQueue bankpubbytes;
        bankpubfile.TransferTo(bankpubbytes);
        RSA::PublicKey bankpubkey;
        bankpubkey.Load(bankpubbytes);

        //Initialize the random number generator
        AutoSeededRandomPool rndA;
        boost::system::error_code EC;

        //Generate the challenge (random number) to send to the Bank
        byte challenge[128];
        rndA.GenerateBlock(challenge,128);
        std::string chaldisp;
        StringSource(challenge, 128, true, 
                                    new HexEncoder(new StringSink(chaldisp)));
	//cout << "\nchal: " << chaldisp << endl;
        //Convert the challenge to an integer and encrypt it with the Bank's
        //public key.
        Integer m ((const byte*)challenge,128);
        Integer c = bankpubkey.ApplyFunction(m);
        
        //Convert the encrypted challenge back into a string.
        std::string challengeenc;
        size_t req = c.MinEncodedSize();
        challengeenc.resize(req);
        c.Encode((byte*)challengeenc.data(),challengeenc.size());
        
        //Base 64 encode the encrypted challenge.
        std::string challenge64;
        StringSource pubss(challengeenc, true, new Base64Encoder(new StringSink(challenge64)));
        challenge64.erase(std::remove(challenge64.begin(),challenge64.end(),'\n'),challenge64.end());
        //std::cout<<"\nchal:"<<challenge64<<std::endl;

        //Send the encrypted challenge to the Bank.
        boost::asio::write(s, boost::asio::buffer(challenge64),
                                boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }

        //If all has gone correctly, receive the same challenge sent to the 
        //Bank, from the Bank. However, this time the challenge is encrypted
        //with the ATM's public key by the Bank.
        boost::asio::streambuf pubresponse;
        boost::asio::read_until(s, pubresponse, "\0");
        std::istream pub_response_stream(&pubresponse);
        std::string pubanswer;
        std::getline(pub_response_stream, pubanswer);
        //std::cout<<"\nchal:"<<pubanswer<<std::endl;
        
        //Decode the received encrypted challenge from Base 64 into non-Base 64
        //format.
        std::string pubdec;
        StringSource pubrss(pubanswer, true, new Base64Decoder(new StringSink(pubdec)));

        //Decrypt the encrypted challenge received from the Bank using the ATM's
        //RSA private key.
        Integer c2((const byte*)pubdec.data(),pubdec.size());
        Integer r = privkey.CalculateInverse(rndA, c2);
        std::string chalrecovered;
        size_t reqq = r.MinEncodedSize();
        chalrecovered.resize(reqq);
        r.Encode((byte *)chalrecovered.data(), chalrecovered.size());
        std::string chaldisp2;
        StringSource(chalrecovered, true, 
                                    new HexEncoder(new StringSink(chaldisp2)));
        //std::cout<<"\nrec:"<<chaldisp2<<std::endl;
        
        //Ensure that the challenge received from the Bank after decryption is
        //the same challenge that was originally generated, encrypted using the
        //Bank's RSA public key, and sent to the Bank.
        if(chaldisp != chaldisp2){
            throw runtime_error("Phony baloney");
        }
        
        //Begin the Diffie-Hellman key exchange.
        DH dhA;

        //Initialize the Diffie-Hellman parameters.
        dhA.AccessGroupParameters().Initialize(p, q, g);

        //Validate the initialized Diffie-Hellman parameters.
        if(!dhA.GetGroupParameters().ValidateGroup(rndA, 3))
            throw runtime_error("Failed to validate prime and generator");

        size_t count = 0;

        Integer v = ModularExponentiation(g, q, p);
        if(v != Integer::One())
            throw runtime_error("Failed to verify order of the subgroup");

        //Create the public and private Diffie-Hellman keys.
        SecByteBlock privA(dhA.PrivateKeyLength());
        SecByteBlock pubA(dhA.PublicKeyLength());
        dhA.GenerateKeyPair(rndA, privA, pubA);

        //Begin exchanging the Diffie-Hellman public keys. Send the ATM's
        //Diffie-Hellman public key to the Bank after Base 64 encoding it.
        std::string pubA64;
        StringSource ss(pubA.data(),pubA.size()+1,true,new Base64Encoder(new StringSink(pubA64)));
        pubA64.erase(std::remove(pubA64.begin(),pubA64.end(),'\n'), pubA64.end());
        //std::cout<<"\nATM dh pub: \n"<<pubA64<<std::endl;
        boost::asio::write(s, boost::asio::buffer(pubA64),
                                boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }
        //Receive the Bank's Diffie-Hellman public key.
        boost::asio::streambuf response;
        boost::asio::read_until(s, response, "\0");
        std::istream response_stream(&response);
        std::string answer;
        std::getline(response_stream, answer);
        //std::cout << "\nBank dh pub: \n"<<answer << std::endl;

        //Base 64 decode the Bank's Diffie-Hellman public key.
        std::string pubBstr;
        StringSource ss2(answer, true, new Base64Decoder(new StringSink(pubBstr)));
        SecByteBlock pubB((byte*)pubBstr.data(),pubBstr.size());

        SecByteBlock sharedA(dhA.AgreedValueLength());

        //Ensure that a shared secret was established.
        if(!dhA.Agree(sharedA, privA, pubB))
            throw runtime_error("Failed to reach shared secret (1A)");

        //Integer a;
	//a.Decode(sharedA.BytePtr(), sharedA.SizeInBytes());
        //cout << "\nShared secret (A): " << std::hex << a << endl;
        
        //Begin using AES CFB
        int aesKeyLength = SHA256::DIGESTSIZE;
        int defBlockSize = AES::BLOCKSIZE;
        SecByteBlock key(SHA256::DIGESTSIZE);
        SHA256().CalculateDigest(key, sharedA, sharedA.size());
        byte iv1[AES::BLOCKSIZE];

        //Get the initial value from the Bank
        boost::asio::streambuf response2;
        boost::asio::read_until(s, response2, "\0");
        std::istream response_stream2(&response2);
        std::string answer2;
        std::getline(response_stream2, answer2);
        std::string iv1string;
        StringSource ssiv1(answer2,true,new Base64Decoder(new StringSink(iv1string)));
        if(iv1string.length()<16){
            throw std::exception();
        }
        for(int i=0;i<16;i++){
            iv1[i]=iv1string[i];
        }
        
        /*
        std::string ivdisp;
        StringSource(iv1, sizeof(iv1), true,
		new HexEncoder(
			new StringSink(ivdisp)
		) // HexEncoder
	); // StringSource
	cout << "iv: " << ivdisp << endl;
        */
        CFB_Mode<AES>::Encryption cfbEncryption(key, aesKeyLength, iv1);
        CFB_Mode<AES>::Decryption cfbDecryption(key, aesKeyLength, iv1);
        //std::cout<<(char*)iv2<<std::endl;
        
        //The handshake has been completed and AES CFB is used from this point
        //onward.
	while (std::getline(std::cin, request)) {
            //Validate the user inputted command using regex (see "validate.h")
            if (!IsValidATMCommand(request)) {
                std::cerr << "INVALID COMMAND" << std::endl;
                continue;
            }
            //Setup login using ATM cards.
            if (request.find("login") == 0){
                int space = request.find(" ") +1;
                int account = 0;
                std::string accountStr = "";
                std::string user = request.substr(space, request.size() - space);
                std::ifstream cardFile(user + ".card");
                if(cardFile.is_open()){
                    cardFile >> account;
                    std::stringstream ss;
                    ss << account;
                    accountStr = ss.str();
                }
                else{
                    std::cerr << "Unable to read card file" << std::endl;
                } 
                cardFile.close();               
                //Replace "login <username>" with "login <account-num>" before
                //sending to the Bank.
                request = "login " + accountStr;
            }
            
            std::string encryptedRequest;
            StringSource es(request, true, new StreamTransformationFilter(cfbEncryption,new StringSink(encryptedRequest)));
            //std::cout<<"\nencrypt:\n"<<encryptedRequest<<std::endl;
            std::string encryptedRequest64;
            StringSource aesEncode(encryptedRequest,true,new Base64Encoder(new StringSink(encryptedRequest64)));
            encryptedRequest64.erase(std::remove(encryptedRequest64.begin(),encryptedRequest64.end(),'\n'), encryptedRequest64.end());
            //std::cout<<"\nencrypt/encode:\n"<<encryptedRequest64<<std::endl;


            boost::system::error_code EC;
            boost::asio::write(s, boost::asio::buffer(encryptedRequest64),
                                boost::asio::transfer_all(), EC);
            if ((boost::asio::error::eof == EC) ||                           
                              (boost::asio::error::connection_reset == EC)) {
                std::cerr << "ERROR" << std::endl;
            }
            //Read and display the Bank's response.
            boost::asio::streambuf response;
            boost::asio::read_until(s, response, "\0");
            std::istream response_stream(&response);
            std::string answer;
            std::getline(response_stream, answer);
            
            std::string data64dec;
            std::string sdata(answer);
            StringSource ss(sdata,true, new Base64Decoder(new StringSink(data64dec)));
            std::string decryptedRequest;
            StringSource ds(data64dec, true, new StreamTransformationFilter(cfbDecryption,new StringSink(decryptedRequest)));
            
            std::cout << /*answer*/ decryptedRequest << std::endl;
        }
    }
    //If anything goes wrong, the Bank will terminate its connection with the
    //ATM.
    catch (std::exception & e){
        std::cerr << "DISCONNECTED FROM BANK" << std::endl;
    }
    return EXIT_SUCCESS;
}
