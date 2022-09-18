#pragma once
#include "runtime/core/meta/reflection/reflection.h"


#include <string>
#include <vector>

namespace Pilot
{
    class Component;

    REFLECTION_TYPE(ComponentDefinitionRes)
    CLASS(ComponentDefinitionRes, Fields)
    {
        REFLECTION_BODY(ComponentDefinitionRes);

    public:
        std::string m_type_name;
        std::string m_component;
    };

    REFLECTION_TYPE(ObjectDefinitionRes)
    CLASS(ObjectDefinitionRes, Fields)
    {
        REFLECTION_BODY(ObjectDefinitionRes);

    public:
        std::vector<Reflection::ReflectionPtr<Component>> m_components;
    };

    REFLECTION_TYPE(ObjectInstanceRes)
    CLASS(ObjectInstanceRes, Fields)
    {
        REFLECTION_BODY(ObjectInstanceRes);

    public:
        std::string  m_name;
        std::string  m_definition;
        
        std::uint32_t              m_id;
        std::uint32_t              m_parent_id;
        std::uint32_t              m_sibling_index;
        std::vector<std::uint32_t> m_chilren_id;


        std::vector<Reflection::ReflectionPtr<Component>> m_instanced_components;
    };
} // namespace Pilot
