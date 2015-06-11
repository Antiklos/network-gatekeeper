#include <stdio.h>

#ifndef CONTRACT_H
#define CONTRACT_H

bool deliver_service(T_STATE *state);

void evaluate_request(T_STATE *state);
bool evaluate_propose(T_STATE *state);
void evaluate_reject(T_STATE *state);

#endif
