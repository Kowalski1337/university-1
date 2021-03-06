#ifndef CPP_NETWORK_REQUEST_H
#define CPP_NETWORK_REQUEST_H

#include <string>
#include <unordered_map>
#include <ostream>

namespace network { namespace http
{
	enum class request_type
	{
		get, post
	};

	class request_line
	{
		request_type type_;
		std::string uri_;
		std::string http_version_;

	public:
		request_line(request_type type, std::string uri, std::string http_version);

		request_type type() const noexcept;

		std::string const &uri() const noexcept;

		std::string const &http_version() const noexcept;
	};

	class request
	{
		request_line line_;
		std::unordered_map<std::string, std::string> headers_;

	public:
		request(request_line request_line,
		        std::unordered_map<std::string, std::string> headers);

		request_line const &line() const noexcept;

		std::unordered_map<std::string, std::string> const &headers() const noexcept;
	};

	std::string to_string(request_type request_type_);

	request_type request_type_from_string(std::string type_str);

	// TODO report bug
	std::string to_string(request_line const &request_line_);

	std::string to_string(request_line const &request_line, std::string host);

	std::string to_string(request const &request_);

	std::string to_string(request const &request, std::string const &host);
}}

#endif //CPP_NETWORK_REQUEST_H
