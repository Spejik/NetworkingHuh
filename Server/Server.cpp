#include <boost/bind/bind.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <string>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;


std::string MakeDaytime()
{
    using namespace boost::posix_time;
    ptime t = microsec_clock::universal_time();
    return to_iso_string(t);
}

class Connection_tcp
    : public boost::enable_shared_from_this<Connection_tcp>
{
public:
    typedef boost::shared_ptr<Connection_tcp> pointer;

    static pointer Create(boost::asio::io_context& io_context)
    {
        return pointer(new Connection_tcp(io_context));
    }

    tcp::socket& Socket()
    {
        return socket_;
    }

    void Start()
    {
        message_ = MakeDaytime();

        boost::asio::async_write(socket_, boost::asio::buffer(message_),
            boost::bind(&Connection_tcp::HandleWrite, shared_from_this()));
    }

private:
    Connection_tcp(boost::asio::io_context& io_context)
        : socket_(io_context)
    {
    }

    void HandleWrite()
    {
    }

    tcp::socket socket_;
    std::string message_;
};

class Server_tcp
{
public:
    Server_tcp(boost::asio::io_context& io_context)
        : io_context_(io_context),
        acceptor_(io_context, tcp::endpoint(tcp::v4(), 13))
    {
        StartAccept();
    }

private:
    void StartAccept()
    {
        Connection_tcp::pointer new_connection =
            Connection_tcp::Create(io_context_);

        acceptor_.async_accept(new_connection->Socket(),
            boost::bind(&Server_tcp::HandleAccept, this, new_connection,
                boost::asio::placeholders::error));
    }

    void HandleAccept(Connection_tcp::pointer new_connection,
        const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->Start();
        }

        StartAccept();
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
};

class Server_udp
{
public:
    Server_udp(boost::asio::io_context& io_context)
        : socket_(io_context, udp::endpoint(udp::v4(), 13))
    {
        StartReceive();
    }

private:
    void StartReceive()
    {
        socket_.async_receive_from(
            boost::asio::buffer(recv_buffer_), remote_endpoint_,
            boost::bind(&Server_udp::HandleReceive, this,
                boost::asio::placeholders::error));
    }

    void HandleReceive(const boost::system::error_code& error)
    {
        if (!error)
        {
            boost::shared_ptr<std::string> message(
                new std::string(MakeDaytime()));

            socket_.async_send_to(boost::asio::buffer(*message), remote_endpoint_,
                boost::bind(&Server_udp::HandleSend, this, message));

            StartReceive();
        }
    }

    void HandleSend(boost::shared_ptr<std::string> /*message*/)
    {
    }

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    std::array<char, 1> recv_buffer_;
};

int main()
{
    try
    {
        boost::asio::io_context io_context;
        Server_tcp server1(io_context);
        Server_udp server2(io_context);
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}