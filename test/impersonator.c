#include <arpa/inet.h>

#include "config.h"

/**
 * Some core functions, as linked from unit testing code.
 */

static char addr_buffer1[INET6_ADDRSTRLEN];
static char addr_buffer2[INET6_ADDRSTRLEN];

char const *
v4addr2str(struct in_addr *addr)
{
	return inet_ntop(AF_INET, addr, addr_buffer1, sizeof(addr_buffer1));
}

char const *
v4addr2str2(struct in_addr *addr)
{
	return inet_ntop(AF_INET, addr, addr_buffer2, sizeof(addr_buffer2));
}

char const *
v6addr2str(struct in6_addr *addr)
{
	return inet_ntop(AF_INET6, addr, addr_buffer1, sizeof(addr_buffer1));
}

char const *
v6addr2str2(struct in6_addr *addr)
{
	return inet_ntop(AF_INET6, addr, addr_buffer2, sizeof(addr_buffer2));
}

char const *
config_get_local_repository(void)
{
	return "repository/";
}

enum sync_strategy
config_get_sync_strategy(void)
{
	return SYNC_ROOT;
}

bool
config_get_color_output(void)
{
	return false;
}


char *
config_get_rsync_program(void)
{
	return "rsync";
}

struct string_array const *
config_get_rsync_args(void)
{
	static const struct string_array array = { 0 };
	return &array;
}
