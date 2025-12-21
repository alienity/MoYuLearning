#include "runtime/function/render/render_resource_loader.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/config_manager/config_manager.h"
#include "runtime/resource/res_type/data/mesh_data.h"

#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_mesh_loader.h"

#include "runtime/platform/file_service/file_service.h"
#include "runtime/platform/file_service/file_system.h"
#include "runtime/platform/file_service/binary_reader.h"
#include "runtime/platform/file_service/binary_writer.h"

#include "runtime/resource//basic_geometry/mesh_tools.h"
#include "runtime/resource/basic_geometry/icosphere_mesh.h"
#include "runtime/resource/basic_geometry/cube_mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYEXR_IMPLEMENTATION
#include <tinyexr.h>

#include <algorithm>
#include <filesystem>
#include <vector>
#include <iostream>  
#include <string>
#include <cctype>
#include <algorithm>

namespace MoYu
{
    void RenderResourceBase::iniUploadBatch(RHI::D3D12Device* device)
    {
        m_Device         = device;
        m_ResourceUpload = device->GetLinkedDevice()->GetResourceUploadBatch();
        m_GraphicsMemory = device->GetLinkedDevice()->GetGraphicsMemory();
    }

    void RenderResourceBase::startUploadBatch() 
    {
        m_ResourceUpload->Begin();
    }

    void RenderResourceBase::endUploadBatch()
    {
        // Upload the resources to the GPU.
        auto uploadResourcesFinished = m_ResourceUpload->End(
            m_Device->GetLinkedDevice()->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct)->GetCommandQueue());

        // Wait for the upload thread to terminate
        uploadResourcesFinished.wait();
    }

    void RenderResourceBase::commitUploadBatch()
    {
        RHI::D3D12SyncHandle syncHandle = m_Device->GetLinkedDevice()
                                              ->GetCommandQueue(RHI::RHID3D12CommandQueueType::Direct)
                                              ->ExecuteCommandLists({}, false);

        m_GraphicsMemory->Commit(syncHandle);
    }
    
    std::future<std::shared_ptr<MoYuScratchImage>> RenderResourceBase::asyncLoadImage(std::string file)
    {
        // Mark resource as loading
        setResourceLoadState(file, ResourceLoadState::Loading);
        
        // Launch async task to load image
        return std::async(std::launch::async, [this, file]() {
            auto result = loadImage(file);
            setResourceLoadState(file, ResourceLoadState::CPULoaded);
            return result;
        });
    }

    std::future<RenderMeshData> RenderResourceBase::asyncLoadMeshData(std::string mesh_file)
    {
        // Mark resource as loading
        setResourceLoadState(mesh_file, ResourceLoadState::Loading);
        
        // Launch async task to load mesh
        return std::async(std::launch::async, [this, mesh_file]() {
            auto result = loadMeshData(mesh_file);
            setResourceLoadState(mesh_file, ResourceLoadState::CPULoaded);
            return result;
        });
    }

    void RenderResourceBase::setResourceLoadState(const std::string& key, ResourceLoadState state)
    {
        std::lock_guard<std::mutex> lock(m_UploadQueueMutex);
        _ResourceLoadStates[key] = state;
    }

    ResourceLoadState RenderResourceBase::getResourceLoadState(const std::string& key)
    {
        std::lock_guard<std::mutex> lock(m_UploadQueueMutex);
        auto it = _ResourceLoadStates.find(key);
        if (it != _ResourceLoadStates.end()) {
            return it->second;
        }
        return ResourceLoadState::NotLoaded;
    }

    void RenderResourceBase::queueResourceForGPUUpload(const std::string& key, std::function<void()> upload_func)
    {
        std::lock_guard<std::mutex> lock(m_UploadQueueMutex);
        m_PendingGPUUploads.push({key, upload_func});
        setResourceLoadState(key, ResourceLoadState::GPULoading);
    }

    bool RenderResourceBase::processPendingGPUUploads()
    {
        bool processedAny = false;
        std::lock_guard<std::mutex> lock(m_UploadQueueMutex);
        
        // Process all pending uploads
        while (!m_PendingGPUUploads.empty()) {
            auto uploadTask = m_PendingGPUUploads.front();
            m_PendingGPUUploads.pop();
            
            // Execute the upload function
            uploadTask.upload_function();
            
            // Mark as uploaded
            _ResourceLoadStates[uploadTask.resource_key] = ResourceLoadState::GPULoaded;
            processedAny = true;
        }
        
        return processedAny;
    }
    
    /*
    std::shared_ptr<MoYuScratchImage> RenderResourceBase::loadTextureHDR(std::string file)
    {
        std::shared_ptr<MoYuScratchImage> texture = _TextureData_Caches[file];

        if (texture == nullptr)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            bool is_hdr = stbi_is_hdr(file.c_str());

            std::string fileFullPath = asset_manager->getFullPath(file).generic_string();

            texture = std::make_shared<MoYuScratchImage>();

            int iw, ih, in;
            in = 4;
            if (is_hdr)
            {
                float* _load_pixels = stbi_loadf(fileFullPath.c_str(), &iw, &ih, &in, 0);
                texture->Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, iw, ih, 1, 1);
                uint8_t* _pixels = texture->GetPixels();
                memcpy(_pixels, _load_pixels, sizeof(float) * 4 * iw * ih);
                stbi_image_free(_load_pixels);
            }
            else
            {
                float* _load_pixels; // width * height * RGBA
                const char* err = nullptr;
                int ret = LoadEXR((float**)&_load_pixels, &iw, &ih, fileFullPath.c_str(), &err);
                if (ret != TINYEXR_SUCCESS)
                {
                    if (err)
                    {
                        fprintf(stderr, "ERR : %s\n", err);
                        FreeEXRErrorMessage(err); // release memory of error message.
                    }
                }
                else
                {
                    texture->Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, iw, ih, 1, 1);
                    uint8_t* _pixels = texture->GetPixels();
                    memcpy(_pixels, _load_pixels, sizeof(float) * 4 * iw * ih);
                    free(_load_pixels);
                }
            }

            _TextureData_Caches[file] = texture;
        }

        return texture;
    }
    */
    /*
    std::shared_ptr<MoYuScratchImage> RenderResourceBase::loadTexture(std::string file)
    {
        std::shared_ptr<MoYuScratchImage> texture = _TextureData_Caches[file];

        if (texture == nullptr)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            texture = std::make_shared<MoYuScratchImage>();

            std::filesystem::path fileFullPath = asset_manager->getFullPath(file);
            MoYu::FileStream stream(fileFullPath, MoYu::FileMode::Open, MoYu::FileAccess::Read);
            std::unique_ptr<std::byte[]> fileDatas = stream.readAll();

            MoYu::MoYuTexMetadata _texMetaData;
            DirectX::LoadFromDDSFile(fileFullPath.c_str(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, _texMetaData, );


            int iw, ih, in;
            texture->m_pixels = stbi_load(fileFullPath.c_str(), &iw, &ih, &in, force_channel);

            if (!texture->m_pixels)
                return nullptr;

            texture->m_width  = iw;
            texture->m_height = ih;
            texture->m_channels = in;
            
            texture->m_depth  = 1;
            texture->m_mip_levels   = 0;
            texture->m_array_layers = 0;

            texture->m_is_hdr = false;
            texture->m_need_free = true;

            _TextureData_Caches[file] = texture;
        }

        return texture;
    }
    */

    std::shared_ptr<MoYuScratchImage> RenderResourceBase::loadImage(std::string file)
    {
        auto _asset = _AssetData_Caches.find(file);
        if (_asset == _AssetData_Caches.end() || _asset->second.type != ResourceType::Texture)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            std::filesystem::path file_full_path = asset_manager->getFullPath(file);
            std::string file_path_str = file_full_path.generic_string();
            std::string file_extension = file_full_path.extension().generic_string();
            
            auto texture = std::make_shared<MoYuScratchImage>();

            int iw, ih, in;
            // No longer preset the number of channels, let stbi automatically detect
            int desired_channels = 0; // 0 means using the native channel count of the image
            
            if (file_extension == ".hdr")
            {
                if (stbi_is_hdr(file.c_str()))
                {
                    //float* _load_pixels = stbi_loadf(file_path_str.c_str(), &iw, &ih, &in, desired_channels);
                    //texture->Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, iw, ih, 1, 1);
                    //uint8_t* _pixels = texture->GetPixels();
                    //memcpy(_pixels, _load_pixels, sizeof(float) * 4 * iw * ih);
                    //stbi_image_free(_load_pixels);
                }
                MoYuScratchImage _scratchImage;
                DirectX::LoadFromHDRFile(file_full_path.c_str(), nullptr, _scratchImage);
                *texture = std::move(_scratchImage);
            }
            else if (file_extension == ".exr")
            {
                float* _load_pixels; // width * height * RGBA
                const char* err = nullptr;
                int ret = LoadEXR((float**)&_load_pixels, &iw, &ih, file_path_str.c_str(), &err);
                if (ret != TINYEXR_SUCCESS)
                {
                    if (err)
                    {
                        fprintf(stderr, "ERR : %s\n", err);
                        FreeEXRErrorMessage(err); // release memory of error message.
                    }
                }
                else
                {
                    texture->Initialize2D(DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, iw, ih, 1, 1);
                    uint8_t* _pixels = texture->GetPixels();
                    memcpy(_pixels, _load_pixels, sizeof(float) * 4 * iw * ih);
                    free(_load_pixels);
                }
            }
            else if (file_extension == ".dds")
            {
                MoYuScratchImage _scratchImage;
                DirectX::LoadFromDDSFile(file_full_path.c_str(), DirectX::DDS_FLAGS::DDS_FLAGS_NONE, nullptr, _scratchImage);

                *texture = std::move(_scratchImage);
            }
            else if (file_extension == ".tga")
            {
                MoYuScratchImage _scratchImage;
                DirectX::LoadFromTGAFile(file_full_path.c_str(), nullptr, _scratchImage);

                *texture = std::move(_scratchImage);
            }
            else
            {
                auto _load_pixels = stbi_load(file_path_str.c_str(), &iw, &ih, &in, desired_channels);
                
                // Select appropriate DXGI format based on actual channel count
                DXGI_FORMAT format;
                switch(in) 
                {
                case 1:
                    format = DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS;
                    break;
                case 2:
                    format = DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS;
                    break;
                case 3:
                    format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS; // STBI converts 3 channels to 4 channels
                    break;
                case 4:
                default:
                    format = DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS;
                    break;
                }

                texture->Initialize2D(format, iw, ih, 1, 1);
                uint8_t* _pixels = texture->GetPixels();
                
                // Calculate the correct copy size
                size_t pixel_size = iw * ih * in * sizeof(unsigned char);
                memcpy(_pixels, _load_pixels, pixel_size);
                stbi_image_free(_load_pixels);
            }

            // Store in unified cache with type information
            AssetResource assetResource;
            assetResource.type = ResourceType::Texture;
            assetResource.data = texture;
            _AssetData_Caches[file] = assetResource;

            return texture;
        }

        return std::get<std::shared_ptr<MoYuScratchImage>>(_asset->second.data);
    }

    RenderMeshData RenderResourceBase::loadMeshData(std::string mesh_file)
    {
        auto _asset = _AssetData_Caches.find(mesh_file);
        if (_asset == _AssetData_Caches.end() || _asset->second.type != ResourceType::Mesh)
        {
            std::shared_ptr<AssetManager> asset_manager = g_runtime_global_context.m_asset_manager;
            ASSERT(asset_manager);

            RenderMeshData ret {};

            if (std::filesystem::path(mesh_file).extension() == ".json")
            {
                std::shared_ptr<MeshData> bind_data = std::make_shared<MeshData>();
                asset_manager->loadAsset<MeshData>(mesh_file, *bind_data);

                ret.m_static_mesh_data.m_InputElementDefinition = D3D12MeshVertexStandard::InputElementDefinition;

                // vertex buffer
                size_t vertex_size = bind_data->vertex_buffer.size() * sizeof(Vertex);
                auto _vertex_buffer = std::make_shared<MoYuScratchBuffer>();
                _vertex_buffer->Initialize(vertex_size);
                memcpy(_vertex_buffer->GetBufferPointer(), bind_data->vertex_buffer.data(), vertex_size);
                ret.m_static_mesh_data.m_vertex_buffer = _vertex_buffer;

                // index buffer
                size_t index_size = bind_data->index_buffer.size() * sizeof(int);
                auto _index_buffer = std::make_shared<MoYuScratchBuffer>();
                _index_buffer->Initialize(index_size);
                memcpy(_index_buffer->GetBufferPointer(), bind_data->index_buffer.data(), index_size);
                ret.m_static_mesh_data.m_index_buffer = _index_buffer;

                MoYu::AABB bounding_box;
                for (size_t i = 0; i < bind_data->vertex_buffer.size(); i++)
                {
                    Vertex v = bind_data->vertex_buffer[i];
                    bounding_box.merge(glm::float3(v.px, v.py, v.pz));
                }
                ret.m_axis_aligned_box = bounding_box;
            }
            else
            {
                //E:\AllTestFolder\PilotLearningTest\engine\source\editor\Debug\asset\objects\basic\sphere.obj

                if (mesh_file.find("sphere") != std::string::npos)
                {
                    MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::Icosphere::ToBasicMesh();
                    ret.m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
                    ret.m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
                }
                else if (mesh_file.find("convexmesh") != std::string::npos)
                {
                    MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::Icosphere::ToBasicMesh(0.5f, 1, false);
                    ret.m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
                    ret.m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
                }
                else if (mesh_file.find("cube") != std::string::npos)
                {
                    MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::CubeMesh::ToBasicMesh();
                    ret.m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
                    ret.m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
                }
                else if (mesh_file.find("triangle") != std::string::npos)
                {
                    MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::TriangleMesh::ToBasicMesh();
                    ret.m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
                    ret.m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
                }
                else if (mesh_file.find("square") != std::string::npos)
                {
                    MoYu::Geometry::BasicMesh _basicMesh = MoYu::Geometry::SquareMesh::ToBasicMesh();
                    ret.m_static_mesh_data = MoYu::Geometry::ToStaticMesh(_basicMesh);
                    ret.m_axis_aligned_box = MoYu::Geometry::ToAxisAlignedBox(_basicMesh);
                }
                else
                {
                    MoYu::AABB bounding_box;
                    ret.m_static_mesh_data = LoadModel(mesh_file, bounding_box);
                    ret.m_axis_aligned_box = bounding_box;
                }
            }

            // Store in unified cache with type information
            AssetResource assetResource;
            assetResource.type = ResourceType::Mesh;
            assetResource.data = ret;
            _AssetData_Caches[mesh_file] = assetResource;

            return ret;
        }
        else
        {
            return std::get<RenderMeshData>(_asset->second.data);
        }
    }

} // namespace MoYu