/*
 * This file is part of RTRlib.
 *
 * This file is subject to the terms and conditions of the MIT license.
 * See the file LICENSE in the top level directory for more details.
 *
 * Website: http://rtrlib.realmv6.org/
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include "rtrlib/rtr/packets.h"

int __wrap_tr_send_all(const struct tr_socket *socket,
		       const void *pdu, const size_t len,
		       const time_t timeout)
{
	return (int)mock();
}

void __wrap_rtr_change_socket_state(struct rtr_socket *rtr_socket,
				    const enum rtr_socket_state new_state)
{
	;
}

void test_rtr_send_reset_query(void)
{
	struct rtr_socket socket;

	socket.connection_state_fp = NULL;

	will_return(__wrap_tr_send_all, 0);
	assert_int_equal(rtr_send_reset_query(&socket), RTR_ERROR);

	will_return(__wrap_tr_send_all, 10);
	assert_int_equal(rtr_send_reset_query(&socket), RTR_SUCCESS);
}

void test_rtr_change_socket_state(void)
{
	struct rtr_socket socket;

	socket.connection_state_fp = NULL;
	socket.state = RTR_RESET;

	__real_rtr_change_socket_state(&socket, RTR_SYNC);
	assert_int_equal(socket.state, RTR_SYNC);

	__real_rtr_change_socket_state(&socket, RTR_SYNC);
	assert_int_equal(socket.state, RTR_SYNC);

	__real_rtr_change_socket_state(&socket, RTR_SHUTDOWN);
	assert_int_equal(socket.state, RTR_SHUTDOWN);

	__real_rtr_change_socket_state(&socket, RTR_ESTABLISHED);
	assert_int_equal(socket.state, RTR_SHUTDOWN);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_rtr_send_reset_query),
		cmocka_unit_test(test_rtr_change_socket_state)
	};
	return cmocka_run_group_tests(tests, NULL, NULL);
}
