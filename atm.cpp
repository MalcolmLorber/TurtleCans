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
#include<boost/asio.hpp>
#include "validate.h"
#include <fstream>
#include <algorithm>
#include <iomanip>

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

int roundUp(int numToRound, int multiple)
{
    if (multiple == 0)
        return numToRound;

    int remainder = numToRound % multiple;
    if (remainder == 0)
        return numToRound;

    return numToRound + multiple - remainder;
}

std::string makeFixedLength(const int i, const int length=4)
{
    std::ostringstream ostr;

    if (i < 0)
        ostr << '-';

    ostr << std::setfill('0') << std::setw(length) << (i < 0 ? -i : i);

    return ostr.str();
}

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

        //handshake
        
        DH dhA;
        AutoSeededRandomPool rndA;

        dhA.AccessGroupParameters().Initialize(p, q, g);

        if(!dhA.GetGroupParameters().ValidateGroup(rndA, 3))
            throw runtime_error("Failed to validate prime and generator");

        size_t count = 0;

        p = dhA.GetGroupParameters().GetModulus();
        q = dhA.GetGroupParameters().GetSubgroupOrder();
        g = dhA.GetGroupParameters().GetGenerator();

        Integer v = ModularExponentiation(g, q, p);
        if(v != Integer::One())
            throw runtime_error("Failed to verify order of the subgroup");

        SecByteBlock privA(dhA.PrivateKeyLength());
        SecByteBlock pubA(dhA.PublicKeyLength());
        dhA.GenerateKeyPair(rndA, privA, pubA);

        //////////////////////////////////////////////////////////////
        //EXCHANGE PUBLICS HERE
        //send(pubA)
        //base64encode it
        std::string pubA64;
        StringSource ss(pubA.data(),pubA.size()+1,true,new Base64Encoder(new StringSink(pubA64)));
        pubA64.erase(std::remove(pubA64.begin(),pubA64.end(),'\n'), pubA64.end());
        std::cout<<"\nATM dh pub: \n"<<pubA64<<std::endl;
        boost::system::error_code EC;
        boost::asio::write(s, boost::asio::buffer(pubA64),
                                boost::asio::transfer_all(), EC);
        if ((boost::asio::error::eof == EC) ||                           
            (boost::asio::error::connection_reset == EC)) {
            std::cerr << "ERROR" << std::endl;
        }
        //recv(pubB)
        boost::asio::streambuf response;
        boost::asio::read_until(s, response, "\0");
        std::istream response_stream(&response);
        std::string answer;
        std::getline(response_stream, answer);
        std::cout << "\nBank dh pub: \n"<<answer << std::endl;
        //base64decode it
        std::string pubBstr;
        StringSource ss2(answer, true, new Base64Decoder(new StringSink(pubBstr)));
        SecByteBlock pubB((byte*)pubBstr.data(),pubBstr.size());
        //////////////////////////////////////////////////////////////

        SecByteBlock sharedA(dhA.AgreedValueLength());

        if(!dhA.Agree(sharedA, privA, pubB))
            throw runtime_error("Failed to reach shared secret (1A)");

        Integer a;
	a.Decode(sharedA.BytePtr(), sharedA.SizeInBytes());
        cout << "\nShared secret (A): " << std::hex << a << endl;

        //////////////////////////////////////////////////////////////

        int aesKeyLength = SHA256::DIGESTSIZE;
        int defBlockSize = AES::BLOCKSIZE;
        SecByteBlock key(SHA256::DIGESTSIZE);
        SHA256().CalculateDigest(key, sharedA, sharedA.size());
        byte iv[AES::BLOCKSIZE]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        //rndA.GenerateBlock(iv, AES::BLOCKSIZE);
        CFB_Mode<AES>::Encryption cfbEncryption(key, aesKeyLength, iv);
        CFB_Mode<AES>::Decryption cfbDecryption(key, aesKeyLength, iv);
        
        //end handshake

	while (std::getline(std::cin, request)) {
            //Validate the user inputted command using regex
            if (!IsValidATMCommand(request)) {
                std::cerr << "INVALID COMMAND" << std::endl;
                continue;
            }

            //setup login with cards
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
                request = "login " + accountStr;
            }

            
            /*char* enc_msg = new char[request.length()+1];
            strcpy(enc_msg,request.c_str());
            cfbEncryption.ProcessData((byte*)enc_msg, (byte*)enc_msg,(int)strlen(enc_msg)+1);
            std::cout<<enc_msg<<std::endl;
            std::string encreq;
            StringSource ss((byte*)enc_msg,request.length()+1,true,new Base64Encoder(new StringSink(encreq)));
            delete [] enc_msg;
            
            std::cout<<encreq<<std::endl;*/

            //char* encryptedRequest = new char[roundUp(request.length(),16)];
            std::string encryptedRequest;
            //cfbEncryption.ProcessData((byte*)encryptedRequest,(const byte*)request.c_str(),request.length()+1);
            StringSource es(request, true, new StreamTransformationFilter(cfbEncryption,new StringSink(encryptedRequest)));
            std::cout<<"\nencrypt:\n"<<encryptedRequest<<std::endl;
            std::string encryptedRequest64;
            StringSource aesEncode((byte*)encryptedRequest.c_str(),encryptedRequest.size()+1/*roundUp(request.length(),16)+2*/,true,new Base64Encoder(new StringSink(encryptedRequest64)));
            encryptedRequest64.erase(std::remove(encryptedRequest64.begin(),encryptedRequest64.end(),'\n'), encryptedRequest64.end());
            std::cout<<"\nencrypt/encode:\n"<<encryptedRequest64<<std::endl;
            
            //delete[] encryptedRequest;
             

            std::string data64dec;
            std::string sdata(encryptedRequest64);
            std::cout<<"\ndata:\n"<<(sdata)<<std::endl<<sdata.size()<<std::endl;
            StringSource ss((byte*)(sdata.c_str()),sdata.size()+1,true, new Base64Decoder(new StringSink(data64dec)));
            std::cout<<"\nreached after decoder\n:"<<data64dec<<std::endl;
            //char* decryptedRequest = new char[msgsize+1];
            std::string decryptedRequest; 
            std::cout<<"\nmessgae length:"<<((int)data64dec.length())<<std::endl;
            //cfbDecryption.ProcessData((byte*)decryptedRequest,(const byte*)data64dec.c_str(),msgsize);
            StringSource ds(data64dec, true, new StreamTransformationFilter(cfbDecryption,new StringSink(decryptedRequest)));
            std::cout<<"\nDec:\n"<<decryptedRequest<<std::endl;



            boost::system::error_code EC;
            boost::asio::write(s, boost::asio::buffer(encryptedRequest64),
                                boost::asio::transfer_all(), EC);
            if ((boost::asio::error::eof == EC) ||                           
                              (boost::asio::error::connection_reset == EC)) {
                std::cerr << "ERROR" << std::endl;
            }
            boost::asio::streambuf response;
            boost::asio::read_until(s, response, "\0");
            std::istream response_stream(&response);
            std::string answer;
            std::getline(response_stream, answer);
            std::cout << answer << std::endl;
        }
    }
    catch (std::exception & e){
        std::cerr << "DISCONNECTED FROM BANK" << std::endl;
    }
    return EXIT_SUCCESS;
}
