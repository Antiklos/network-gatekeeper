#include <stdio.h>

#ifndef CONTRACT_H
#define CONTRACT_H

bool deliver_service(T_STATE *state, T_CONFIG *config);

void evaluate_request(T_STATE *state, T_CONFIG *config);
bool evaluate_propose(T_STATE *state, T_CONFIG *config);

#endif
