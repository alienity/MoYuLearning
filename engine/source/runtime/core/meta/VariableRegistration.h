// VariableRegistration.h
#pragma once
#include "VariableRegistry.h"

namespace VariableSystem
{
    template <typename T>
    class VariableRegistration
    {
    public:
        VariableRegistration(const std::string& name, T* ptr)
        {
            VariableRegistry::Get().Register(name, ptr);
        }
    };
} // namespace VariableSystem
