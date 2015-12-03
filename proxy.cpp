/*******************************************************************************
Name: Samuel Wenninger, Brandon Drumheller, and Malcolm Lorber
Date Created: 12-02-2015
Filename: proxy.cpp
*******************************************************************************/

//Standard library
#include <cstdlib>
#include <cstddef>
#include <iostream>
#include <string>
//Boost
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread/mutex.hpp>

/*******************************************************************************
 @DESC: The bridge class holds all of the classes need to read and write both
        upstream and downstream.
 @ARGS: N/A
 @RTN : N/A
*******************************************************************************/
namespace tcp_proxy {
namespace ip = boost::asio::ip;
class bridge : public boost::enable_shared_from_this<bridge> {
    public:
        typedef ip::tcp::socket socket_type;
        typedef boost::shared_ptr<bridge> ptr_type;
        bridge(boost::asio::io_service& ios)
            : downstream_socket_(ios),
            upstream_socket_(ios) {}
        //Accessor functions for the downstream and upstream sockets.
        socket_type& downstream_socket() { return downstream_socket_; }
        socket_type& upstream_socket() { return upstream_socket_; }
        //Start everything
        void start(const std::string& upstream_host, unsigned short upstream_port) {
            upstream_socket_.async_connect(
                    ip::tcp::endpoint(
                        boost::asio::ip::address::from_string(upstream_host),
                        upstream_port),
                    boost::bind(&bridge::handle_upstream_connect,
                        shared_from_this(),
                        boost::asio::placeholders::error));
        }
        void handle_upstream_connect(const boost::system::error_code& error) {
            if (!error) {
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_,max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));

                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_,max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }
    private:
        void handle_downstream_write(const boost::system::error_code& error) {
            if (!error) {
                upstream_socket_.async_read_some(
                        boost::asio::buffer(upstream_data_,max_data_length),
                        boost::bind(&bridge::handle_upstream_read,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }
        void handle_downstream_read(const boost::system::error_code& error,
                const size_t& bytes_transferred) {
            if (!error) {
                async_write(upstream_socket_,
                        boost::asio::buffer(downstream_data_,bytes_transferred),
                        boost::bind(&bridge::handle_upstream_write,
                            shared_from_this(),
                            boost::asio::placeholders::error));
            }
            else
                close();
        }
        void handle_upstream_write(const boost::system::error_code& error) {
            if (!error) {
                downstream_socket_.async_read_some(
                        boost::asio::buffer(downstream_data_,max_data_length),
                        boost::bind(&bridge::handle_downstream_read,
                            shared_from_this(),
                            boost::asio::placeholders::error,
                            boost::asio::placeholders::bytes_transferred));
            }
            else
                close();
        }
        void handle_upstream_read(const boost::system::error_code& error,
                const size_t& bytes_transferred) {
            if (!error) {
                async_write(downstream_socket_,
                        boost::asio::buffer(upstream_data_,bytes_transferred),
                        boost::bind(&bridge::handle_downstream_write,
                            shared_from_this(),
                            boost::asio::placeholders::error));
            }
            else
                close();
        }
        //Close both the upstream and downstream socket if they are open.
        void close() {
            boost::mutex::scoped_lock lock(mutex_);
            if (downstream_socket_.is_open())
                downstream_socket_.close();
            if (upstream_socket_.is_open())
                upstream_socket_.close();
        }
        socket_type downstream_socket_;
        socket_type upstream_socket_;
        //Max size for a single message. If a message received is larger than
        //this, it will be split into multiple pieces.
        enum { max_data_length = 8192 }; //8KB
        unsigned char downstream_data_[max_data_length];
        unsigned char upstream_data_[max_data_length];
        boost::mutex mutex_;
    public:
        //The acceptor class is used to accept incoming TCP connections
        class acceptor {
            public:
                acceptor(boost::asio::io_service& io_service,
                        const std::string& local_host, unsigned short local_port,
                        const std::string& upstream_host, unsigned short upstream_port)
                    : io_service_(io_service),
                    localhost_address(boost::asio::ip::address_v4::from_string(local_host)),
                    acceptor_(io_service_,ip::tcp::endpoint(localhost_address,local_port)),
                    upstream_port_(upstream_port),
                    upstream_host_(upstream_host) {}

                bool accept_connections() {
                    try {
                        session_ = boost::shared_ptr<bridge>(new bridge(io_service_));
                        acceptor_.async_accept(session_->downstream_socket(),
                                boost::bind(&acceptor::handle_accept,
                                    this,
                                    boost::asio::placeholders::error));
                    }
                    catch(std::exception& e) {
                        std::cerr << "acceptor exception: " << e.what() << std::endl;
                        return false;
                    }
                    return true;
                }
            private:
                void handle_accept(const boost::system::error_code& error) {
                    if (!error) {
                        session_->start(upstream_host_,upstream_port_);
                        if (!accept_connections()) {
                            std::cerr << "Failure during call to accept." << std::endl;
                        }
                    }
                    else {
                        std::cerr << "Error: " << error.message() << std::endl;
                    }
                }
                boost::asio::io_service& io_service_;
                ip::address_v4 localhost_address;
                ip::tcp::acceptor acceptor_;
                ptr_type session_;
                unsigned short upstream_port_;
                std::string upstream_host_;
        };

};
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "usage: ./a.out <atm port> <bank port>" << std::endl;
        return 1;
    }
    const unsigned short local_port   = static_cast<unsigned short>(::atoi(argv[1]));
    const unsigned short forward_port = static_cast<unsigned short>(::atoi(argv[2]));
    const std::string local_host      = "127.0.0.1";
    const std::string forward_host    = "127.0.0.1";
    boost::asio::io_service ios;
    try {
        tcp_proxy::bridge::acceptor acceptor(ios,
                local_host, local_port,
                forward_host, forward_port);
        acceptor.accept_connections();
        ios.run();
    }
    catch(std::exception& e) {
        std::cerr << "ERROR" << std::endl;
        return 1;
    }
    return 0;
}
