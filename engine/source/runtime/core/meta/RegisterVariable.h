// RegisterVariable.h
#pragma once
#include "VariableRegistration.h"

#define REGISTER_VARIABLE(type, name, value) \
type name = value; \
static VariableSystem::VariableRegistration<type> reg_##name(#name, &name)

namespace VariableSystem
{
    void ExecuteRegisterVariableStr(const std::string& input);
}
