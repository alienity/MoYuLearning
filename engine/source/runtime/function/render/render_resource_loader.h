#pragma once

#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/render_swap_context.h"
#include "runtime/function/render/render_common.h"

#include "runtime/function/render/rhi/d3d12/d3d12_graphicsMemory.h"
#include "runtime/function/render/rhi/d3d12/d3d12_resourceUploadBatch.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <variant>

namespace MoYu
{
    class RenderScene;
    class RenderCamera;

    enum class ResourceType
    {
        Texture,
        Buffer,
        Mesh
    };

    enum class ResourceLoadState
    {
        NotLoaded,
        Loading,
        CPULoaded,
        GPULoading,
        GPULoaded,
        Error
    };

    struct AssetResource
    {
        ResourceType type;
        std::variant<
            std::shared_ptr<MoYuScratchImage>,
            std::shared_ptr<MoYuScratchBuffer>,
            RenderMeshData
        > data;

        // Helper functions to access data safely
        std::shared_ptr<MoYuScratchImage> getAsTexture() const {
            return std::get<std::shared_ptr<MoYuScratchImage>>(data);
        }

        std::shared_ptr<MoYuScratchBuffer> getAsBuffer() const {
            return std::get<std::shared_ptr<MoYuScratchBuffer>>(data);
        }

        RenderMeshData getAsMesh() const {
            return std::get<RenderMeshData>(data);
        }
    };

    struct PendingResourceUpload
    {
        std::string resource_key;
        std::function<void()> upload_function;
    };

    class RenderResourceBase
    {
    public:
        robin_hood::unordered_map<std::string, AssetResource> _AssetData_Caches;
        robin_hood::unordered_map<std::string, ResourceLoadState> _ResourceLoadStates;

    public:
        RenderResourceBase() = default;

        virtual ~RenderResourceBase() {}

        void iniUploadBatch(RHI::D3D12Device* device);

        void startUploadBatch();
        void endUploadBatch();
        void commitUploadBatch();

        // Asynchronous resource loading
        std::future<std::shared_ptr<MoYuScratchImage>> asyncLoadImage(std::string file);
        std::future<RenderMeshData> asyncLoadMeshData(std::string mesh_file);
        
        // Resource state management
        void setResourceLoadState(const std::string& key, ResourceLoadState state);
        ResourceLoadState getResourceLoadState(const std::string& key);

        //std::shared_ptr<MoYuScratchImage> loadTextureHDR(std::string file);
        //std::shared_ptr<MoYuScratchImage> loadTexture(std::string file);
        std::shared_ptr<MoYuScratchImage> loadImage(std::string file);

        RenderMeshData loadMeshData(std::string mesh_file);

        // Queue resource for GPU upload
        void queueResourceForGPUUpload(const std::string& key, std::function<void()> upload_func);
        bool processPendingGPUUploads();

    //protected:
        RHI::D3D12Device*         m_Device;

    protected:
        RHI::ResourceUploadBatch* m_ResourceUpload;
        RHI::GraphicsMemory*      m_GraphicsMemory;
        
        // Pending GPU uploads
        std::queue<PendingResourceUpload> m_PendingGPUUploads;
        std::mutex m_UploadQueueMutex;
    };
} // namespace MoYu