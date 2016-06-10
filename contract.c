#include <stdio.h>

#include "contract.h"

bool deliver_service(T_STATE *state) {
  time_t current_time = time(NULL);
  if (state->time_expiration < current_time) return false;

  long int packets_outstanding = state->payment_advance + state->packets_delivered;
  int64_t payment_outstanding = packets_outstanding*state->price;
  return (payment_outstanding < state->payment_sent);
}

bool renew_service(T_STATE *state) {
  time_t current_time = time(NULL);
  if (state->time_expiration > current_time + TIME_TO_TX_CONFIRM) return false;

  long int expected_packets_delivered = state->packets_delivered + (TIME_TO_TX_CONFIRM*PACKET_THROUGHPUT);
  long int packets_outstanding = state->payment_advance + expected_packets_delivered;
  int64_t payment_outstanding = packets_outstanding*state->price;
  return (payment_outstanding >= state->payment_sent);
}

void evaluate_request(T_STATE *state) {
  state->price = 5;
  state->payment_advance = 5;
  state->time_expiration = time(NULL) + 60;
}

bool evaluate_propose(T_STATE *state) {
  if (state->price < 5) {
    return true;
  } else {
    state->price = 3;
    return false;
  }
}

void evaluate_reject(T_STATE *state) {
  state->price = 4;
}

