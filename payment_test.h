#include <stdio.h>

#define PAYMENT_INTERFACE_TEST_IDENTIFIER 0

#ifndef PAYMENT_TEST_H

#define PAYMENT_TEST_H

T_PAYMENT_INTERFACE payment_test_interface();

void payment_test_init();

void send_payment_test();

void payment_test_destroy();

#endif
