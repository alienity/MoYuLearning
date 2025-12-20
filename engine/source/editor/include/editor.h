#pragma once

#include "runtime/core/math/moyu_math2.h"

#include <memory>

namespace MoYu
{
    class EditorUI;
    class MoYuEngine;

    class MoYuEditor
    {
        friend class EditorUI;

    public:
        MoYuEditor();
        virtual ~MoYuEditor();

        void initialize(MoYuEngine* engine_runtime);
        void clear();

        void logicalTick(float delta_time);
        void rendererTick();

    protected:
        std::shared_ptr<EditorUI> m_editor_ui;
        MoYuEngine* m_engine_runtime {nullptr};
    };
} // namespace MoYu
