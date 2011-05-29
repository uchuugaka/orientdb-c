#include "o_native_socket_selector.h"
#include "o_memory.h"
#include "o_list.h"
#include "o_native_socket_internal.h"
#include "o_native_lock.h"
#include <sys/socket.h>

struct o_native_socket_selector
{
	enum selector_mode mode;
	struct o_list * socks;
	struct o_native_lock * select_lock;
	struct o_native_lock * socks_lock;
	struct o_list * active_sock;
};

int o_native_socket_selector_fill_set(struct o_native_socket_selector * selector, fd_set * set)
{
	o_native_lock_lock(selector->socks_lock);
	struct o_list_iterator *i = o_list_begin(selector->socks);
	if (i != 0)
	{
		do
		{
			struct o_native_socket * sock = (struct o_native_socket *) o_list_iterator_current(i);
			FD_SET(o_native_socket_internal_descriptor(sock),set);
		} while (o_list_iterator_next(i));
	}
	int size = o_list_size(selector->socks);
	o_native_lock_unlock(selector->socks_lock);
	return size;
}

void o_native_socket_selector_fill_active(struct o_native_socket_selector * selector, fd_set * set)
{
	o_native_lock_lock(selector->socks_lock);
	struct o_list_iterator *i = o_list_begin(selector->socks);
	if (i != 0)
	{
		do
		{
			struct o_native_socket * sock = (struct o_native_socket *) o_list_iterator_current(i);
			if (FD_ISSET(o_native_socket_internal_descriptor(sock),set))
				o_list_add(selector->active_sock, sock);
		} while (o_list_iterator_next(i));
	}
	o_native_lock_unlock(selector->socks_lock);
}

struct o_native_socket_selector * o_native_socket_selector_new(enum selector_mode mode)
{
	struct o_native_socket_selector * selector = o_malloc(sizeof(struct o_native_socket_selector));
	selector->mode = mode;
	selector->socks = o_list_new();
	selector->socks_lock = o_native_lock_new();
	selector->select_lock = o_native_lock_new();
	selector->active_sock = o_list_new();
	return selector;
}

void o_native_socket_selector_add_socket(struct o_native_socket_selector * selector, struct o_native_socket* socket)
{
	o_native_lock_lock(selector->socks_lock);
	o_list_add(selector->socks, socket);
	o_native_lock_unlock(selector->socks_lock);
}

void o_native_socket_selector_remove_socket(struct o_native_socket_selector * selector, struct o_native_socket* socket)
{
	o_native_lock_lock(selector->socks_lock);
	o_list_remove(selector->socks, socket);
	o_native_lock_unlock(selector->socks_lock);
}

struct o_native_socket* o_native_socket_selector_select(struct o_native_socket_selector * selector, int timeout)
{
	struct o_native_socket * return_sock = 0;
	o_native_lock_lock(selector->select_lock);
	if (o_list_size(selector->active_sock) == 0)
	{
		fd_set set;
		FD_ZERO(&set);
		int size = o_native_socket_selector_fill_set(selector, &set);
		int select_ret;
		switch (selector->mode)
		{
		case READ:
			select_ret = select(size, &set, 0, 0, 0);
			break;
		case WRITE:
			select_ret = select(size, 0, &set, 0, 0);
			break;
		case ERROR:
			select_ret = select(size, 0, 0, &set, 0);
			break;
		}
		if (select_ret > 0)
		{
			o_native_socket_selector_fill_active(selector, &set);
		}
	}
	if (o_list_size(selector->active_sock) > 0)
	{
		return_sock = (struct o_native_socket *) o_list_get(selector->active_sock, 0);
		o_list_remove(selector->active_sock, return_sock);
	}
	o_native_lock_unlock(selector->select_lock);
	return return_sock;
}

void o_native_socket_selector_free(struct o_native_socket_selector * selector)
{
	o_list_free(selector->socks);
	o_list_free(selector->socks_lock);
	o_list_free(selector->select_lock);
	o_list_free(selector->active_sock);
	o_free(selector);
}
