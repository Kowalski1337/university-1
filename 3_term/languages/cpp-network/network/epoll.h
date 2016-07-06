#ifndef CPP_NETWORK_EPOLL_H
#define CPP_NETWORK_EPOLL_H

#include <functional>
#include "file_descriptor.h"

namespace network
{
	using callback = std::function<void()>;

	class epoll_registration
	{
		friend class epoll;

		callback on_read, on_write, on_close;

	public:
		void set_on_read(callback on_read);

		void set_on_write(callback on_write);

		void set_on_close(callback on_close);

		void unset_on_read();

		void unset_on_write();

		void unset_on_close();
	};

	class epoll : public base_descriptor_resource
	{
		bool is_stopped = false;

	public:
		epoll(file_descriptor &&fd) noexcept;

		epoll();

		epoll(epoll &&rhs) noexcept;

		void add(const file_descriptor &fd, epoll_registration const &registration);

		void mod(const file_descriptor &fd, epoll_registration const &registration);

		void del(const file_descriptor &fd);

		void run();

		void soft_stop() noexcept;
	};
}

#endif //CPP_NETWORK_EPOLL_H