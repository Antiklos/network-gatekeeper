#include <stdio.h>

#include "contract.h"

bool deliver_service(T_STATE *state) {
  time_t current_time = time(NULL);
  if (state->time_expiration < current_time) return false;

  return (state->payment_advance + state->account->balance > 0);
}

void evaluate_request(T_STATE *state, T_CONFIG *config) {
  state->price = config->default_price;
  state->payment_advance = config->grace_period_price;
  state->time_expiration = time(NULL) + 60;
}

bool evaluate_propose(T_STATE *state, T_CONFIG *config) {
  if (state->price <= config->default_price) {
    return true;
  } else {
    return false;
  }
}

