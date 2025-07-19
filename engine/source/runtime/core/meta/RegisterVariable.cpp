#include "RegisterVariable.h"
#include "core/log/log_system.h"
#include <algorithm>
#include <sstream>

// REGISTER_VARIABLE(int, myInt, 42);
// REGISTER_VARIABLE(float, myFloat, 3.14f);
// REGISTER_VARIABLE(std::string, myString, "Hello");

namespace VariableSystem
{
    std::string toLower(const std::string& str)
    {
        std::string lowerStr = str;
        std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return lowerStr;
    }

    std::vector<std::string> SplitInput(const std::string& input)
    {
        std::vector<std::string> result;
        std::string word;
        for (char c : input)
        {
            if (c == ' ')
            {
                if (!word.empty())
                {
                    result.push_back(word);
                    word.clear();
                }
            }
            else
            {
                word += c;
            }
        }
        if (!word.empty())
        {
            result.push_back(word);
        }
        return result;
    }

    void ExecuteRegisterVariableStr(const std::string& input)
    {
        if (input.empty()) return;

        std::vector<std::string> tokens = SplitInput(input);

        if (tokens.size() >= 2)
        {
            std::string varName = tokens[0];
            std::string valueStr = tokens[1];

            const VariableRegistry::VariableInfo* info = VariableRegistry::Get().GetVariableInfo(varName);

            if (!info)
            {
                LOG_INFO("Error: Variable not found: " + varName);
                return;
            }

            std::string varType = info->type;

            if (varType == typeid(int).name())
            {
                try
                {
                    int val = std::stoi(valueStr);
                    *reinterpret_cast<int*>(info->ptr) = val;
                    LOG_INFO("Set " + varName + " to " + std::to_string(val));
                }
                catch (...)
                {
                    LOG_INFO("Error: Invalid integer value for " + varName);
                }
            }
            else if (varType == typeid(float).name())
            {
                try
                {
                    float val = std::stof(valueStr);
                    *reinterpret_cast<float*>(info->ptr) = val;
                    LOG_INFO("Set " + varName + " to " + std::to_string(val));
                }
                catch (...)
                {
                    LOG_INFO("Error: Invalid float value for " + varName);
                }
            }
            else if (varType == typeid(std::string).name())
            {
                *reinterpret_cast<std::string*>(info->ptr) = valueStr;
                LOG_INFO("Set " + varName + " to " + valueStr);
            }
            else if (varType == typeid(bool).name())
            {
                std::string lowerValue = toLower(valueStr);
                bool val = (lowerValue == "true" || lowerValue == "1");
                *reinterpret_cast<bool*>(info->ptr) = val;
                LOG_INFO("Set " + varName + " to " + (val ? "true" : "false"));
            }
            else
            {
                LOG_INFO("Error: Unsupported variable type for " + varName);
            }
        }
        else
        {
            LOG_INFO("Usage: set <varName> <value>");
        }
    }
}
