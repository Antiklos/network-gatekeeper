#include <stdio.h>

#include "contract.h"

void evaluate_request(T_STATE *state) {
  state->price = 5;
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

