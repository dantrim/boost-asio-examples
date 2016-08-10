#include <typeinfo>
#include <boost/array.hpp>
#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>

#define MAXBUFLEN 65507

using boost::asio::ip::udp;

namespace ip = boost::asio::ip;


class udp_socket
{
    public :
        udp_socket(std::string name, boost::shared_ptr<boost::asio::io_service> io_service, int port_to_bind_to) :
            m_name(name),
            m_socket(*io_service, ip::udp::endpoint(ip::udp::v4(), port_to_bind_to)),
            m_port(port_to_bind_to),
            m_buf(),
            m_message_count(0)
        {
            udp::endpoint ep = m_socket.local_endpoint();
            std::cout << "udp_socket init" << std::endl;

            if(m_socket.is_open()) {
                std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << " is open" << std::endl;
            }
            else {
                std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << " is closed" << std::endl;
            }

            // start listening
            start_receive();
        }

        ////////////////////////////////////////////////////////////////////////
        void start_receive()
        {
            m_socket.async_receive_from(
                boost::asio::buffer(m_buf), m_remote_endpoint,
                boost::bind(&udp_socket::handle_receive, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
            );

        } // startrecieve
        ////////////////////////////////////////////////////////////////////////
        void handle_receive(const boost::system::error_code error, std::size_t size_)
        {
            std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << "    incoming data packet of from " << m_remote_endpoint.data() << "  of size: " << size_ << " bytes" << std::endl;
            std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << "    >>> " << m_buf.data() << std::endl;
            m_message_count++;

            start_receive();
            
        } // handlereceive

    private :
        std::string m_name;
        udp::socket m_socket;
        int m_port;
        udp::endpoint m_remote_endpoint;
        boost::array<char, MAXBUFLEN> m_buf;
        int m_message_count;

}; // class


int main()
{
    boost::shared_ptr<boost::asio::io_service> io_service(
        new boost::asio::io_service);
    boost::shared_ptr< boost::asio::io_service::work> work (
        new boost::asio::io_service::work(*io_service));

    std::string name = "receiverport";
    int daqport = 1234;
    udp_socket recever(name, io_service, daqport);

    std::cout << "MAIN THREAD: " << boost::this_thread::get_id() << std::endl;

    io_service->run();


    return 0;
}
