#include <stdio.h>

#include "contract.h"

void evaluate_request(T_CONTRACT *contract) {
  contract->price = 5;
}

bool evaluate_propose(T_CONTRACT *contract) {
  return true;
}

void evaluate_reject(T_CONTRACT *contract) {
  contract->price = 4;
}

