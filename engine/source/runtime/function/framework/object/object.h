#pragma once

#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/function/framework/object/object_id_allocator.h"
#include "runtime/resource/res_type/common/object.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"
#include "runtime/core/math/moyu_math2.h"

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <cstdint>

namespace MoYu
{
	class Level;

	enum GObjectStatus
	{
		GO_Init,
		GO_Dirty,
		GO_Idle,
		GO_Erase,
		GO_None
	};

	/// GObject : Game Object base class
	class GObject : public std::enable_shared_from_this<GObject>
	{
	public:
		GObject();
		GObject(std::shared_ptr<Level> level);
		GObject(MoYu::GObjectID id, std::shared_ptr<Level> level);
		virtual ~GObject();

		virtual void preTick(float delta_time);
		virtual void tick(float delta_time);
		virtual void lateTick(float delta_time);

		virtual bool isRootNode() const { return m_id == MoYu::K_Root_Object_Id; }

		virtual bool load(const MoYu::ObjectInstanceRes& object_instance_res);
		virtual void save(MoYu::ObjectInstanceRes& out_object_instance_res);

		virtual void setID(MoYu::GObjectID id) { m_id = id; }
		virtual MoYu::GObjectID getID() const { return m_id; }

		virtual std::shared_ptr<GObject> getParent() const;
		virtual std::vector<std::shared_ptr<GObject>> getChildren() const;

		virtual void setSiblingIndex(int sibling_index) { m_sibling_index = sibling_index; }
		virtual int  getSiblingIndex() const { return m_sibling_index; }

		virtual void setParent(MoYu::GObjectID parentID, std::optional<std::uint32_t> sibling_index = std::nullopt);
		virtual void removeChild(MoYu::GObjectID childID);

		virtual bool isInit() const { return m_status == MoYu::GObjectStatus::GO_Init; }
		virtual bool isDirty() const { return m_status == MoYu::GObjectStatus::GO_Init || m_status == MoYu::GObjectStatus::GO_Dirty; }
		virtual bool isToErase() const { return m_status == MoYu::GObjectStatus::GO_Erase; }
		virtual bool isIdle() const { return m_status == MoYu::GObjectStatus::GO_Idle; }
		virtual bool isNone() const { return m_status == MoYu::GObjectStatus::GO_None; }

		virtual void markInit() { m_status = MoYu::GObjectStatus::GO_Init; }
		virtual void markDirty() { m_status = MoYu::GObjectStatus::GO_Dirty; }
		virtual void markToErase();
		virtual void markIdle() { m_status = MoYu::GObjectStatus::GO_Idle; }
		virtual void markNone() { m_status = MoYu::GObjectStatus::GO_None; }

		virtual void setName(std::string name) { m_name = name; }
		virtual const std::string& getName() const { return m_name; }

		virtual std::shared_ptr<class TransformComponent> getTransformComponent() { return m_transform_component; }
		virtual std::weak_ptr<class TransformComponent> getTransformComponentWeak() { return m_transform_component; }

		virtual bool hasComponent(const std::string& compenent_type_name) const;

		virtual std::vector<std::shared_ptr<class Component>> getComponents() { return m_components; }

		template<typename TComponent>
		std::shared_ptr<TComponent> tryAddComponent(std::shared_ptr<TComponent> newComponent)
		{
			// Skip adding TransformComponent since it's built-in
			if constexpr (std::is_same_v<TComponent, TransformComponent>)
			{
				LOG_WARN("TransformComponent is built-in and cannot be added manually");
				return nullptr;
			}

			for (size_t i = 1; i < m_components.size(); i++) // Start from index 1 to skip TransformComponent
			{
				if (m_components[i]->getTypeName() == newComponent->getTypeName())
				{
					LOG_INFO("object {} already has component {}", m_name, m_components[i]->getTypeName());
					return nullptr;
				}
			}
			newComponent->setParentNode(shared_from_this());
			m_components.push_back(newComponent);

			return newComponent;
		}

		template<typename TComponent>
		bool tryRemoveComponent(std::shared_ptr<TComponent> toDelComponent)
		{
			// Prevent removal of TransformComponent (index 0)
			if (m_transform_component == toDelComponent)
			{
				LOG_WARN("Cannot remove built-in TransformComponent");
				return false;
			}

			int index_finded = -1;
			for (size_t i = 1; i < m_components.size(); i++) // Start from index 1 to skip TransformComponent
			{
				if (m_components[i]->getComponentId() == toDelComponent->getComponentId())
				{
					index_finded = i;
					break;
				}
			}
			if (index_finded != -1)
			{
				m_components.erase(m_components.begin() + index_finded);
				return true;
			}
			else
			{
				return false;
			}
		}

		template<typename TComponent>
		std::shared_ptr<TComponent> tryGetComponent(const std::string& compenent_type_name)
		{
			// Special handling for TransformComponent
			if (compenent_type_name == TransformComponent::getTypeName())
			{
				return std::static_pointer_cast<TComponent>(m_transform_component);
			}

			for (size_t i = 1; i < m_components.size(); i++) // Start from index 1 to skip TransformComponent
			{
				if (m_components[i]->getTypeName() == compenent_type_name)
				{
					return std::static_pointer_cast<TComponent>(m_components[i]);
				}
			}
			return nullptr;
		}

	protected:
		friend class Level;

		GObjectStatus m_status{ MoYu::GObjectStatus::GO_Init };

		MoYu::GObjectID              m_id{ MoYu::K_Invalid_Object_Id };
		MoYu::GObjectID              m_parent_id{ MoYu::K_Root_Object_Id };
		std::uint32_t                m_sibling_index{ 0 };
		std::vector<MoYu::GObjectID> m_chilren_id{};

		std::weak_ptr<Level> m_current_level;

		std::string m_name;
		//std::string m_definition_url;

		// Transform component as the first component (index 0)
		std::shared_ptr<class TransformComponent> m_transform_component;

		// Other components start from index 1
		// we have to use the ReflectionPtr due to that the components need to be reflected 
		// in editor, and it's polymorphism
		std::vector<std::shared_ptr<class Component>> m_components;
	};
} // namespace MoYu
