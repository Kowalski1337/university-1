#include "file_descriptor.h"
#include "network_exception.h"
#include "utils/log.h"

#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <iostream>

namespace network
{
	utils::log log{std::cout};

	file_descriptor::file_descriptor(int fd)
			: fd_(fd)
	{
		log(utils::info) << "file_descriptor(" << *this << ")\n";
	}

	file_descriptor::file_descriptor(file_descriptor &&rhs) noexcept
			: fd_(rhs.fd_)
	{
		rhs.fd_ = -1;
	}

	file_descriptor::~file_descriptor() noexcept
	{
		if (fd_ == -1)
			return;

		log(utils::info) << "~file_descriptor(" << *this << "), ";
		int error = 0;
		socklen_t err_len = sizeof error;
		if (getsockopt(fd_, SOL_SOCKET, SO_ERROR, static_cast<void *>(&error), &err_len) == 0)
			log(utils::info) << "last error: " << strerror(error);
		log(utils::info) << "\n";

		close(fd_);
	}

	file_descriptor &file_descriptor::operator=(file_descriptor &&rhs) noexcept
	{
		std::swap(fd_, rhs.fd_);
		return *this;
	}

	int file_descriptor::get_raw_fd() const noexcept
	{
		return fd_;
	}

	bool file_descriptor::is_set() const noexcept
	{
		return fd_ != -1;
	}

	void make_non_blocking(file_descriptor const &fd)
	{
		int flags;
		check_return_code(
				flags = fcntl(fd.get_raw_fd(), F_GETFL, 0));
		check_return_code(
				fcntl(fd.get_raw_fd(), F_SETFL, flags | O_NONBLOCK));
	}

	base_descriptor_resource::base_descriptor_resource(network::file_descriptor &&fd) noexcept
			: fd_(std::move(fd))
	{}

	file_descriptor const &base_descriptor_resource::get_fd() const noexcept
	{
		return fd_;
	}
}