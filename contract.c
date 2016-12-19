#include <stdio.h>

#include "contract.h"

bool evaluate_request(T_STATE *state, T_CONFIG *config) {
  if (state->price > 0) {
    return false;
  }
  state->price = config->default_price;
  state->time_expiration = time(NULL) + config->contract_time;
  return true;
}

bool evaluate_propose(T_STATE *state, T_CONFIG *config) {
  if (state->price <= config->default_price) {
    return true;
  } else {
    return false;
  }
}

