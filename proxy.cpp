/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 11-10-2015
Filename: proxy.cpp
Description: Proxy program that only transports data to and from the bank
             program using the Boost libarary. Compile using the following:
             "g++ -std=c++11 proxy.cpp -lboost_system"
*******************************************************************************/

#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
//#include <boost/thread/thread.hpp>

using boost::asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(tcp::socket Socket1, tcp::socket Socket2) 
            : socket_(std::move(Socket1)),
              bank_socket_(std::move(Socket2)) {}
        void Start() {
            DoRead();
        }
    private:
        void DoRead() {
            auto Self(shared_from_this());    
            socket_.async_read_some(boost::asio::buffer(data_, max_length),
            [this, Self](boost::system::error_code EC, std::size_t Length) {
                if (!EC) {
                    DoWrite(Length);
                }
            });
        }

        void DoWrite(std::size_t Length) {
            auto Self(shared_from_this());
            boost::asio::async_write(bank_socket_, boost::asio::buffer(data_, Length),
            [this, Self](boost::system::error_code EC, std::size_t){
                if (!EC) {
                    DoRead();
                }
            });
        }
        tcp::socket socket_;
        tcp::socket bank_socket_;
        enum {max_length = 1024};
        char data_[max_length];
};

/*******************************************************************************
 @DESC: Listen on ATM port and connect to the bank port using the boost:asio
        library. Multiple connections must be able to be handled at the same
        time (multiple ATMs connected to the bank at any given time).
 @ARGS: io_service, port to listen on, port to connect to the bank
 @RTN:  true on success, false on failure
*******************************************************************************/
class Server {
    public:
        Server(boost::asio::io_service & IOService, int ListenPort, int BankPort)
            //Call acceptor constructor with the bank's port as the endopoint 
            //and assign result to acceptor_
            : listen_acceptor_(IOService, tcp::endpoint(tcp::v4(), ListenPort)),
              bank_acceptor_(IOService, tcp::endpoint(tcp::v4(), BankPort)),
              listen_socket_(IOService),
              bank_socket_(IOService) {
                DoAccept();
              }
    private:
        void DoAccept() {
            //Accept connection and start a session by calling the Session
            //constructor
            bank_acceptor_.async_accept(bank_socket_, [this](boost::system::error_code EC1) {
                if (!EC1) {
                    listen_acceptor_.async_accept(listen_socket_, [this](boost::system::error_code EC) {
                        if (!EC) {
                        std::make_shared<Session>(std::move(listen_socket_), std::move(bank_socket_)) -> Start();
                        }
                        DoAccept();
                    });
                }
            });
        }
        tcp::acceptor listen_acceptor_;
        tcp::socket listen_socket_;
        tcp::acceptor bank_acceptor_;
        tcp::socket bank_socket_;
};


/*******************************************************************************
 @DESC: Call appropriate function(s) to initiate communication between the ATM
        and the bank. 
 @ARGS: port to listen on, port to connect to the bank
 @RTN:  EXIT_SUCCESS on success, EXIT_FAILURE on failure
*******************************************************************************/
int main (int argc, char* argv[]) {
    try {
        if (argc != 3) {
            std::cout << "Incorrect number of arguments. Proper usage ./a.out"
                            " <port-to-listen-on> <bank-port>" << std::endl;
            return EXIT_FAILURE;
        }
        boost::asio::io_service IOService;
        int ListenPort = std::stoi(argv[1]);
        int BankPort = std::stoi(argv[2]);
        Server S(IOService, ListenPort, BankPort);
        IOService.run();
    }
    catch (std::exception & e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return EXIT_SUCCESS;
}
