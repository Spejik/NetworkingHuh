#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <string>
#include <memory>
#include <utility>

using boost::asio::ip::tcp;
using boost::asio::ip::udp;


std::string MakeDaytime()
{
    using namespace boost::posix_time;
    ptime t = microsec_clock::universal_time();
    return to_iso_string(t);
}

class Connection_tcp
    : public std::enable_shared_from_this<Connection_tcp>
{
public:
    Connection_tcp(tcp::socket socket)
        : socket_(std::move(socket))
    {
    }

    void Start()
    {
        DoRead();
    }

private:
    void DoRead()
    {
        BOOST_LOG_TRIVIAL(debug) << "tcp_conn.DoRead";
        auto self(shared_from_this());

        socket_.async_read_some(boost::asio::buffer(datain_, 256),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (!ec) DoWrite(length);
            });
    }

    void DoWrite(std::size_t /*length*/)
    {
        BOOST_LOG_TRIVIAL(debug) << "tcp_conn.DoWrite";
        auto self(shared_from_this());
        std::string message = MakeDaytime();

        boost::asio::async_write(socket_, boost::asio::buffer(message),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (ec) DoRead();
            });

    }

private:
    tcp::socket socket_;
    std::string datain_;
};

class Server_tcp
{
public:
    Server_tcp(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        DoAccept();
    }

private:
    void DoAccept()
    {
        BOOST_LOG_TRIVIAL(debug) << "tcp.DoAccept";

        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec) 
                    std::make_shared<Connection_tcp>(std::move(socket))->Start();

                DoAccept();
            });
    }

    tcp::acceptor acceptor_;
};

class Server_udp
{
public:
    Server_udp(boost::asio::io_context& io_context, short port)
        : socket_(io_context, udp::endpoint(udp::v4(), port))
    {
        DoReceive();
    }

private:
    void DoReceive()
    {
        BOOST_LOG_TRIVIAL(debug) << "udp.DoReceive";

        socket_.async_receive_from(boost::asio::buffer(recv_buffer_, 256), remote_endpoint_,
            [this](boost::system::error_code ec, std::size_t bytes_recvd)
            {
                if (!ec && bytes_recvd > 0)
                {
                    BOOST_LOG_TRIVIAL(debug) << "udp received: '" << recv_buffer_ << '\'';
                    DoSend(bytes_recvd);
                }
                else
                    DoReceive();
            });
    }

    void DoSend(std::size_t /*length*/)
    {
        BOOST_LOG_TRIVIAL(debug) << "udp.DoSend";
        std::string message = MakeDaytime();

        socket_.async_send_to(boost::asio::buffer(message, message.size()), remote_endpoint_,
            [this](boost::system::error_code /*ec*/, std::size_t /*bytes_sent*/)
            {
                DoReceive();
            });
    }

    udp::socket socket_;
    udp::endpoint remote_endpoint_;
    std::string recv_buffer_;
};

int main()
{
    try
    {
        boost::asio::io_context io_context;
        short port = 13;
        Server_tcp server1{ io_context, port };
        Server_udp server2{ io_context, port };
        io_context.run();
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL(error) << e.what();
    }

    return 0;
}