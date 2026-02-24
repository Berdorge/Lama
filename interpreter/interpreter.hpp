#ifndef INTERPRETER_HPP
#define INTERPRETER_HPP

#include "bytefile.hpp"

void interpret(bytefile* bf, uint8_t* initial_ip, aint* stack);

#endif
