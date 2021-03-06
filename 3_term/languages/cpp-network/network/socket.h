#ifndef CPP_NETWORK_SERVER_SOCKET_H
#define CPP_NETWORK_SERVER_SOCKET_H

#include "file_descriptor.h"
#include "utils/string_view.h"

#include <stdint.h>

#include <string>
#include <vector>

namespace network
{
	class ipv4_address
	{
		uint32_t address_;

	public:
		ipv4_address(uint32_t address) noexcept;

		uint32_t get_raw_address() const noexcept;
	};

	ipv4_address make_address_any() noexcept;

	std::string to_string(ipv4_address address);


	class ipv4_endpoint
	{
		ipv4_address address_;
		uint16_t port_n_;

		ipv4_endpoint(ipv4_address address, uint16_t port_n) noexcept;

		friend ipv4_endpoint make_ipv4_endpoint_h(ipv4_address address, uint16_t port_h) noexcept;

		friend ipv4_endpoint make_ipv4_endpoint_n(ipv4_address address, uint16_t port_n) noexcept;

		friend ipv4_endpoint make_any_endpoint(ipv4_address address) noexcept;

		friend ipv4_endpoint make_local_endpoint(uint16_t port_h) noexcept;

	public:
		ipv4_address get_address() const noexcept;

		uint16_t get_port_n() const noexcept;

		uint16_t get_port_h() const noexcept;
	};

	ipv4_endpoint make_ipv4_endpoint_h(ipv4_address address, uint16_t port_h) noexcept;

	ipv4_endpoint make_ipv4_endpoint_n(ipv4_address address, uint16_t port_n) noexcept;

	ipv4_endpoint make_any_endpoint(ipv4_address address = make_address_any()) noexcept;

	ipv4_endpoint make_local_endpoint(uint16_t port_h) noexcept;

	std::string to_string(ipv4_endpoint endpoint);


	std::vector<ipv4_endpoint> get_hosts(std::string const &host_name);

	size_t const BUFFER_SIZE = 8192;

	class client_socket : public base_descriptor_resource
	{
	public:
		client_socket(file_descriptor &&fd) noexcept;

		client_socket(client_socket &&rhs) noexcept;

		void assert_availability();

		std::string read();

		size_t write(utils::string_view const &str);
	};

	file_descriptor connect(std::vector<ipv4_endpoint> const &endpoints, bool non_blocking);


	class server_socket : public base_descriptor_resource
	{
	public:
		server_socket(file_descriptor &&fd) noexcept;

		server_socket(server_socket &&rhs) noexcept;

		server_socket(ipv4_endpoint endpoint);

		client_socket accept();
	};


	ipv4_endpoint get_socket_endpoint(file_descriptor const &fd);
}

#endif //CPP_NETWORK_SERVER_SOCKET_H
