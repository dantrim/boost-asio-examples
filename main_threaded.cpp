#include <typeinfo>
#include <unistd.h>
#include <boost/array.hpp>
#include <stdio.h>
#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <boost/atomic.hpp>

#define MAXBUFLEN 65507

using boost::asio::ip::udp;

namespace ip = boost::asio::ip;

boost::mutex global_stream_lock;

void WorkerThread(boost::shared_ptr< boost::asio::io_service > io_service)
{
    global_stream_lock.lock();
    std::cout << "[" << boost::this_thread::get_id() << "]  Thread start" << std::endl;
    global_stream_lock.unlock();
    io_service->run();
    global_stream_lock.lock();
    std::cout << "[" << boost::this_thread::get_id() << "]  Thread finish" << std::endl;
    global_stream_lock.unlock();
}

class udp_socket
{
    public :
        udp_socket(std::string name, boost::shared_ptr<boost::asio::io_service> io_service,
                    boost::shared_ptr<boost::asio::io_service::strand> strand, int port_to_bind_to) :
            m_service(io_service),
            m_strand(strand),
            m_name(name),
            m_socket(*io_service, ip::udp::endpoint(ip::udp::v4(), port_to_bind_to)),
            m_port(port_to_bind_to),
            m_buf(),
            m_message_count(0)
            //m_message_count(new int())
        {
            udp::endpoint ep = m_socket.local_endpoint();
            std::cout << "udp_socket init" << std::endl;

            if(m_socket.is_open()) {
                std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << " is open" << std::endl;
            }
            else {
                std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << " is closed" << std::endl;
            }

            m_message_count = 0;


            for(int i = 0; i < 3; i++) {
                worker_threads.create_thread(boost::bind(&WorkerThread, io_service));
            }
            //io_service->post(boost::bind(&udp_socket::start_receive, this));
            m_strand->post(boost::bind(&udp_socket::start_receive, this));


            // start listening
            //start_receive();
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
            std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << "    incoming data packet of from " << m_remote_endpoint.data() << "  of size: " << size_ << " bytes" << "   msg count: " << m_message_count << std::endl;
            std::cout << "udp_socket[" << boost::this_thread::get_id() << "] " << m_name << "    >>> " << m_buf.data() << std::endl;
            //(m_message_count)++;
            m_message_count.fetch_add(1, boost::memory_order_relaxed);

            //boost::this_thread::sleep(boost::posix_time::seconds(1));


            start_receive();
            
        } // handlereceive

        void close_socket()
        {
            m_socket.close();

            boost::system::error_code ec;
            m_socket.shutdown(boost::asio::ip::udp::socket::shutdown_both, ec);

            m_service->stop();
            worker_threads.join_all();
        }

    private :
        boost::shared_ptr<boost::asio::io_service> m_service;
        boost::shared_ptr<boost::asio::io_service::strand> m_strand;
        std::string m_name;
        udp::socket m_socket;
        int m_port;
        udp::endpoint m_remote_endpoint;
        boost::array<char, MAXBUFLEN> m_buf;
        boost::atomic<int> m_message_count;
        //int m_message_count;
        //boost::shared_ptr<int> m_message_count;
        boost::thread_group worker_threads;

}; // class


int main()
{
    boost::shared_ptr<boost::asio::io_service> io_service(
        new boost::asio::io_service);
    boost::shared_ptr< boost::asio::io_service::work> work (
        new boost::asio::io_service::work(*io_service));

    boost::shared_ptr< boost::asio::io_service::strand> strand(
        new boost::asio::io_service::strand(*io_service));

    std::string name = "receiverport";
    int daqport = 1234;
    udp_socket recever(name, io_service, strand, daqport);

    std::cout << "MAIN THREAD: " << boost::this_thread::get_id() << std::endl;

    //io_service->run();
    std::cin.get();
    recever.close_socket();

    return 0;
}
