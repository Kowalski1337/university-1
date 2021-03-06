#include "socket.h"
#include "network_exception.h"
#include "utils/log.h"

#include <arpa/inet.h>
#include <netdb.h>

#include <array>

namespace network
{
	ipv4_address::ipv4_address(uint32_t address) noexcept
			: address_{address}
	{}

	uint32_t network::ipv4_address::get_raw_address() const noexcept
	{
		return address_;
	}

	ipv4_address make_address_any() noexcept
	{
		return ipv4_address{INADDR_ANY};
	}

	std::string to_string(ipv4_address address)
	{
		in_addr tmp{address.get_raw_address()};
		return inet_ntoa(tmp);
	}

	ipv4_endpoint::ipv4_endpoint(network::ipv4_address address, uint16_t port_n) noexcept
			: address_{address}
			, port_n_{port_n}
	{}

	ipv4_address ipv4_endpoint::get_address() const noexcept
	{
		return address_;
	}

	uint16_t ipv4_endpoint::get_port_n() const noexcept
	{
		return port_n_;
	}

	uint16_t ipv4_endpoint::get_port_h() const noexcept
	{
		return ntohs(port_n_);
	}

	ipv4_endpoint make_ipv4_endpoint_h(ipv4_address address, uint16_t port_h) noexcept
	{
		return ipv4_endpoint{address, htons(port_h)};
	}

	ipv4_endpoint make_ipv4_endpoint_n(ipv4_address address, uint16_t port_n) noexcept
	{
		return ipv4_endpoint{address, port_n};
	}

	ipv4_endpoint make_any_endpoint(ipv4_address address) noexcept
	{
		return ipv4_endpoint{address, 0};
	}

	ipv4_endpoint make_local_endpoint(uint16_t port_h) noexcept
	{
		return ipv4_endpoint{make_address_any(), htons(port_h)};
	}

	std::string to_string(ipv4_endpoint endpoint)
	{
		return to_string(endpoint.get_address()) + ":" + std::to_string(endpoint.get_port_h());
	}

	std::vector<ipv4_endpoint> get_hosts(std::string const &name)
	{
		log(utils::info) << "getting hosts of " << name << "\n";

		addrinfo hints;
		addrinfo *info;

		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int code;
		if ((code = getaddrinfo(name.c_str(), "http", &hints, &info)) != 0)
			throw network_exception{gai_strerror(code)};

		std::vector<ipv4_endpoint> endpoints{};
		try
		{
			for (addrinfo *p = info; p != nullptr; p = p->ai_next)
			{
				sockaddr_in *address = (sockaddr_in *) p->ai_addr;
				endpoints.push_back(make_ipv4_endpoint_n(address->sin_addr.s_addr, address->sin_port));
			}
		}
		catch (std::exception const &exception)
		{
			freeaddrinfo(info);
			throw exception;
		}

		freeaddrinfo(info);

		log(utils::info) << "get_hosts returned [ ";
		for (auto &endpoint : endpoints)
		{
			log(utils::info) << to_string(endpoint) << " ";
		}
		log(utils::info) << "]\n";

		return endpoints;
	}

	client_socket::client_socket(file_descriptor &&fd) noexcept
			: base_descriptor_resource{std::move(fd)}
	{}

	client_socket::client_socket(client_socket &&rhs) noexcept
			: client_socket{std::move(rhs.fd_)}
	{}

	void client_socket::assert_availability()
	{
		log(utils::info) << "asserting availability of " << fd_ << "\n";
		int error = 0;
		socklen_t err_len = sizeof error;
		check_return_code(
				getsockopt(fd_.get_raw_fd(), SOL_SOCKET, SO_ERROR, static_cast<void *>(&error), &err_len));
		if (error != 0)
			throw network_exception(strerror(errno));
	}

	std::string client_socket::read()
	{
		std::array<char, BUFFER_SIZE> buf;
		ssize_t read_n = ::recv(fd_.get_raw_fd(), buf.begin(), buf.size(), MSG_NOSIGNAL);
		check_return_code(read_n);
		std::string string{buf.begin(), static_cast<size_t>(read_n)};

		#ifdef CPP_NETWORK_SOCKET_DEBUG
		log(utils::verbose) << "read " << std::to_string(string.size()) << " bytes from " << fd_ << "\n";
		#endif

		return string;
	}

	size_t client_socket::write(utils::string_view const &str)
	{
		ssize_t written = ::send(fd_.get_raw_fd(), str.begin(), str.size(), MSG_NOSIGNAL);
		check_return_code(written);

		#ifdef CPP_NETWORK_SOCKET_DEBUG
		log(utils::verbose) << "written " << std::to_string(written) << " bytes to " << fd_ << "\n";
		#endif

		return static_cast<size_t>(written);
	}

	file_descriptor connect(std::vector<ipv4_endpoint> const &endpoints, bool non_blocking)
	{
		log(utils::info) << "connecting to " << to_string(endpoints.front()) << "...\n";
		file_descriptor fd{check_return_code(::socket(AF_INET, SOCK_STREAM, 0))};
		if (non_blocking)
			make_non_blocking(fd);
		sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = endpoints.front().get_address().get_raw_address();
		address.sin_port = endpoints.front().get_port_n();
		int const code = ::connect(fd.get_raw_fd(), reinterpret_cast<sockaddr const *>(&address), sizeof address);
		if (errno != EINPROGRESS)
			check_return_code(code);
		return fd;
	}

	server_socket::server_socket(file_descriptor &&fd) noexcept
			: base_descriptor_resource{std::move(fd)}
	{}

	server_socket::server_socket(server_socket &&rhs) noexcept
			: server_socket{std::move(rhs.fd_)}
	{}

	server_socket::server_socket(ipv4_endpoint endpoint)
			: server_socket{file_descriptor{check_return_code(::socket(AF_INET, SOCK_STREAM, 0))}}
	{
		log(utils::info) << "starting server at " << to_string(endpoint) << "\n";

		int enable = 1;
		setsockopt(fd_.get_raw_fd(), SOL_SOCKET, SO_REUSEPORT, &enable, sizeof enable);

		struct sockaddr_in address;
		address.sin_family = AF_INET;
		address.sin_addr.s_addr = endpoint.get_address().get_raw_address();
		address.sin_port = endpoint.get_port_n();

		check_return_code(
				bind(fd_.get_raw_fd(), reinterpret_cast<sockaddr *>(&address), sizeof address));

		check_return_code(
				listen(fd_.get_raw_fd(), SOMAXCONN));
	}

	client_socket server_socket::accept()
	{
		log(utils::info) << "accepting at " << fd_ << "...\n";
		int new_fd = ::accept(fd_.get_raw_fd(), nullptr, nullptr);
		check_return_code(new_fd);
		client_socket accepted{file_descriptor{new_fd}};
		return accepted;
	}

	ipv4_endpoint get_socket_endpoint(file_descriptor const &fd)
	{
		sockaddr_in address;
		socklen_t tmp = sizeof address;
		check_return_code(
				getsockname(fd.get_raw_fd(), reinterpret_cast<sockaddr *>(&address), &tmp));
		return make_ipv4_endpoint_n(address.sin_addr.s_addr, address.sin_port);
	}
}
