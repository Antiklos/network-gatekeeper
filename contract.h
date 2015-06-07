#include <stdio.h>

#ifndef CONTRACT_H
#define CONTRACT_H

void evaluate_request(T_CONTRACT *contract);
bool evaluate_propose(T_CONTRACT *contract);
void evaluate_reject(T_CONTRACT *contract);

#endif
