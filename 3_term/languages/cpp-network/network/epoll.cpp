#include "epoll.h"
#include "network_exception.h"

#include <sys/epoll.h>

#include <forward_list>
#include <iostream>

namespace network
{
	epoll_registration::epoll_registration(file_descriptor const &fd) noexcept : raw_fd{fd.get_raw_fd()}
	{
	}

	void epoll_registration::set_on_read(callback on_read)
	{
		this->on_read = on_read;
	}

	void epoll_registration::set_on_write(callback on_write)
	{
		this->on_write = on_write;
	}

	void epoll_registration::set_on_close(callback on_close)
	{
		this->on_close = on_close;
	}

	void epoll_registration::set_cleanup(callback cleanup)
	{
		this->cleanup = cleanup;
	}

	void epoll_registration::unset_on_read()
	{
		this->on_read = nullptr;
	}

	void epoll_registration::unset_on_write()
	{
		this->on_write = nullptr;
	}

	void epoll_registration::unset_on_close()
	{
		this->on_close = nullptr;
	}

	void epoll_registration::unset_cleanup()
	{
		this->cleanup = nullptr;
	}

	epoll::epoll(file_descriptor &&fd) noexcept : base_descriptor_resource{std::move(fd)}
	{
	}

	epoll::epoll() : epoll{file_descriptor{check_return_code(::epoll_create1(0))}}
	{
	}

	epoll::epoll(epoll &&rhs) noexcept : epoll{std::move(rhs.fd)}
	{
	}

	void epoll::add(epoll_registration &registration)
	{
		epoll_event event;
		event.data.ptr = static_cast<void *>(&registration);
		event.events = (registration.on_read != nullptr ? EPOLLIN : 0u) |
		               (registration.on_write != nullptr ? EPOLLOUT : 0u) |
		               EPOLLRDHUP;
		check_return_code(
				epoll_ctl(this->fd.get_raw_fd(), EPOLL_CTL_ADD, registration.raw_fd, &event));
	}

	void epoll::update(epoll_registration &registration)
	{
		epoll_event event;
		event.data.ptr = static_cast<void *>(&registration);
		event.events = (registration.on_read != nullptr ? EPOLLIN : 0u) |
		               (registration.on_write != nullptr ? EPOLLOUT : 0u) |
		               EPOLLRDHUP;
		check_return_code(
				epoll_ctl(this->fd.get_raw_fd(), EPOLL_CTL_MOD, registration.raw_fd, &event));
	}

	void epoll::remove(epoll_registration &registration)
	{
		check_return_code(
				epoll_ctl(this->fd.get_raw_fd(), EPOLL_CTL_DEL, registration.raw_fd, nullptr));
	}

	void epoll::schedule_cleanup(epoll_registration &registration)
	{
		cleanup_set.insert(&registration);
	}

	void epoll::run()
	{
		is_running = true;
		while (is_running)
		{
			std::array<epoll_event, 10> events;
			int event_n = epoll_wait(fd.get_raw_fd(), events.begin(), static_cast<int>(events.size()), -1);
			check_return_code(event_n);

			for (int event_i = 0; event_i < event_n; ++event_i)
			{
				epoll_registration *registration = static_cast<epoll_registration *>(events[event_i].data.ptr);
				try
				{
					if (events[event_i].events & EPOLLIN)
						registration->on_read();
					if (events[event_i].events & EPOLLOUT)
						registration->on_write();
					if (events[event_i].events & ~(EPOLLIN | EPOLLOUT))
						if (registration->on_close != nullptr)
							registration->on_close();
				}
				catch (std::exception &exception)
				{
					this->schedule_cleanup(*registration);
				}
			}

			for (auto registration : cleanup_set)
				if (registration->cleanup != nullptr)
					try
					{
						registration->cleanup();
					}
					catch (std::exception &exception)
					{
						std::cerr << "Exception during cleanup: " << exception.what() << "\n";
					}
			cleanup_set.clear();
		}
	}

	void epoll::soft_stop() noexcept
	{
		is_running = false;
	}
}
