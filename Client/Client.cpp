#define OLC_PGE_APPLICATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WIN32_WINNT _WIN32_WINNT_WIN10
#include "olcPixelGameEngine.h"

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio.hpp>
#include <boost/log/trivial.hpp>

#include <array>
#include <chrono>
#include <iostream>
#include <string>

namespace asio = boost::asio;
using asio::ip::tcp;
using asio::ip::udp;

class NetworkClient : public olc::PixelGameEngine
{
public:
	NetworkClient()
	{
		// Name your application
		sAppName = "Networking, huh? [Client]";
	}

private:
	asio::io_context io_ctx;
	tcp::socket socket_tcp{ io_ctx };
	udp::socket socket_udp{ io_ctx };
	tcp::resolver resolver_tcp{ io_ctx };
	udp::resolver resolver_udp{ io_ctx };
	tcp::resolver::results_type endpoint_tcp = resolver_tcp.resolve("localhost", "daytime");
	udp::endpoint endpoint_udp = *resolver_udp.resolve(asio::ip::udp::v4(), "localhost", "daytime").begin();

	bool TCP_()
	{
		try 
		{
			std::array<char, 128> buf;
			boost::system::error_code ec;
			size_t wrote = socket_tcp.write_some(boost::asio::buffer("ayo"), ec);
			size_t read = socket_tcp.read_some(boost::asio::buffer(buf), ec);

			if (ec == boost::asio::error::eof)
				return false; // Connection closed cleanly by peer.
			else if (ec)
				throw boost::system::system_error(ec); // Some other error.

			BOOST_LOG_TRIVIAL(info) << "tcp data: " << std::string(buf.data(), read);
			DrawStringDecal({ 10, 10 }, std::string(buf.data(), read), olc::WHITE, { 2.0f, 2.0f });
		}
		catch (std::exception& e)
		{
			BOOST_LOG_TRIVIAL(error) << "TCP: " << e.what();
		}

		return true;
	}

	bool UDP_()
	{
		try
		{
			udp::socket socket(io_ctx);
			socket.open(udp::v4());

			std::string send_buf = "hello???";
			socket.send_to(boost::asio::buffer(send_buf), endpoint_udp);

			std::array<char, 128> recv_buf;
			udp::endpoint sender_endpoint;
			size_t len = socket.receive_from(
				boost::asio::buffer(recv_buf), sender_endpoint);

			BOOST_LOG_TRIVIAL(info) << "udp data: " << std::string(recv_buf.data(), len);
			DrawStringDecal({ 10, 50 }, std::string(recv_buf.data(), len), olc::WHITE, { 2.0f, 2.0f });
		}
		catch (std::exception& e)
		{
			BOOST_LOG_TRIVIAL(error) << "UDP: " << e.what();
		}

		return true;
	}

private:
	bool use_tcp = true;
	bool use_udp = true;

public:
	bool OnUserCreate() override
	{
		asio::connect(socket_tcp, endpoint_tcp);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);
		if (GetKey(olc::Key::T).bPressed) use_tcp = !use_tcp;
		if (GetKey(olc::Key::U).bPressed) use_udp = !use_udp;
		if (use_tcp && !TCP_()) BOOST_LOG_TRIVIAL(warning) << "TCP returned false";
		if (use_udp && !UDP_()) BOOST_LOG_TRIVIAL(warning) << "UDP returned false";

		DrawStringDecal({ 500, 10 }, "tcp", use_tcp ? olc::GREEN : olc::RED, { 2.0f, 2.0f });
		DrawStringDecal({ 500, 50 }, "udp", use_udp ? olc::GREEN : olc::RED, { 2.0f, 2.0f });

		return true;
	}
};

int main()
{
	NetworkClient app;
	if (app.Construct(720, 720, 1, 1))
		app.Start();
	std::cin.get();
	return 0;
}