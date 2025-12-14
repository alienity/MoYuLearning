// VariableRegistry.h
#pragma once
#include <string>
#include <unordered_map>
#include <typeinfo>

namespace VariableSystem
{
    class VariableRegistry
    {
    public:
        static VariableRegistry& Get()
        {
            static VariableRegistry instance;
            return instance;
        }

        struct VariableInfo
        {
            std::string name;
            std::string type;
            void* ptr;
        };

        template <typename T>
        void Register(const std::string& name, T* ptr)
        {
            VariableInfo info;
            info.name = name;
            info.type = typeid(T).name(); // Get type name (compile-time type)
            info.ptr = reinterpret_cast<void*>(ptr); // Pointer to the variable
            variables[name] = info;
        }

        const VariableInfo* GetVariableInfo(const std::string& name) const
        {
            auto it = variables.find(name);
            if (it != variables.end())
            {
                return &it->second;
            }
            return nullptr;
        }

        std::unordered_map<std::string, VariableInfo> variables;
    };
} // namespace VariableSystem
