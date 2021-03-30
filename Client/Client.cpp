#define OLC_PGE_APPLICATION
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "olcPixelGameEngine.h"

#include <boost/asio.hpp>

#include <array>
#include <iostream>

namespace asio = boost::asio;

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
	asio::ip::tcp::socket socket_tcp{ io_ctx };
	asio::ip::udp::socket socket_udp{ io_ctx };
	asio::ip::tcp::resolver resolver_tcp{ io_ctx };
	asio::ip::udp::resolver resolver_udp{ io_ctx };
	asio::ip::tcp::resolver::results_type endpoint_tcp = resolver_tcp.resolve("localhost", "daytime");
	asio::ip::udp::endpoint endpoint_udp = *resolver_udp.resolve(asio::ip::udp::v4(), "localhost", "daytime").begin();

	bool TCP_()
	{
		using asio::ip::tcp;

		std::array<char, 128> buf;
		boost::system::error_code ec;
		size_t len = socket_tcp.read_some(boost::asio::buffer(buf), ec);

		if (ec == boost::asio::error::eof)
			return false; // Connection closed cleanly by peer.
		else if (ec)
			throw boost::system::system_error(ec); // Some other error.

		std::cout << "tcp: " << std::string(buf.data(), len) << std::endl;
		DrawStringDecal({ 10, 10 }, std::string(buf.data(), len));

		return true;
	}

	bool UDP_()
	{
		using asio::ip::udp;

		udp::socket socket(io_ctx);
		socket.open(udp::v4());

		std::array<char, 1> send_buf = { { 0 } };
		socket.send_to(boost::asio::buffer(send_buf), endpoint_udp);

		std::array<char, 128> recv_buf;
		udp::endpoint sender_endpoint;
		size_t len = socket.receive_from(
			boost::asio::buffer(recv_buf), sender_endpoint);

		std::cout << "udp: " << std::string(recv_buf.data(), len) << std::endl;
		DrawStringDecal({50, 10}, std::string(recv_buf.data(), len));

		return true;
	}

public:
	bool OnUserCreate() override
	{
		asio::connect(socket_tcp, endpoint_tcp);

		return true;
	}

	bool OnUserUpdate(float fElapsedTime) override
	{
		Clear(olc::BLACK);
		if (!TCP_() || !UDP_());
			//return false;
		return true;
	}
};

int main()
{
	NetworkClient app;
	if (app.Construct(720, 720, 1, 1, false, true))
		app.Start();
	std::cin.get();
	return 0;
}