#ifndef UTIL_H
#define UTIL_H

enum EVENT {
  MESSAGE_REQUEST = 0,
  MESSAGE_PROPOSE = 1,
  MESSAGE_ACCEPT = 2,
  MESSAGE_REJECT = 3,
  MESSAGE_BEGIN = 4,
  MESSAGE_STOP = 5,
  PAYMENT_RECEIVED = 6,
  PACKETS_SENT = 7
};

int send_message(
);

#endif


