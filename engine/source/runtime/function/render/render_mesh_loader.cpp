#include "render_mesh_loader.h"
#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

#include "mikktspace.h"

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags


struct ModelLoaderMesh
{
    Pilot::AxisAlignedBox                        bounding_box_;
    std::vector<Pilot::MeshVertexDataDefinition> vertexs_;
    std::vector<std::uint16_t>                   indices_;
};

struct MeshLoaderNode
{
    Pilot::AxisAlignedBox        bounding_box_;
    std::vector<ModelLoaderMesh> meshes_;
    std::vector<MeshLoaderNode>  children_;
};

class MittTangentsHelper
{
public:
    // MittTangentsHelper();
    static void calc(ModelLoaderMesh* mesh_);

private:
    static int get_vertex_index(const SMikkTSpaceContext* context, int iFace, int iVert);

    static int  get_num_faces(const SMikkTSpaceContext* context);
    static int  get_num_vertices_of_face(const SMikkTSpaceContext* context, int iFace);
    static void get_position(const SMikkTSpaceContext* context, float* outpos, int iFace, int iVert);

    static void get_normal(const SMikkTSpaceContext* context, float* outnormal, int iFace, int iVert);

    static void get_tex_coords(const SMikkTSpaceContext* context, float* outuv, int iFace, int iVert);

    static void
    set_tspace_basic(const SMikkTSpaceContext* context, const float* tangentu, float fSign, int iFace, int iVert);
};

void            ProcessNode(aiNode* node, const aiScene* scene, MeshLoaderNode& meshes_);
ModelLoaderMesh ProcessMesh(aiMesh* mesh);
void            LoadModelNormal(std::string filename, MeshLoaderNode& meshes_);

void RecursiveLoad(MeshLoaderNode&                               meshes_,
                   std::vector<Pilot::MeshVertexDataDefinition>& vertexs_,
                   std::vector<std::uint16_t>&                   indices_)
{
    for (size_t i = 0; i < meshes_.meshes_.size(); i++)
    {
        ModelLoaderMesh& curmesh_ = meshes_.meshes_[i];
        vertexs_.insert(vertexs_.end(), curmesh_.vertexs_.begin(), curmesh_.vertexs_.end());
        indices_.insert(indices_.end(), curmesh_.indices_.begin(), curmesh_.indices_.end());
    }
    for (size_t i = 0; i < meshes_.children_.size(); i++)
    {
        RecursiveLoad(meshes_.children_[i], vertexs_, indices_);

        Pilot::AxisAlignedBox sub_bounding_box;

    }
}

Pilot::StaticMeshData LoadModel(std::string filename, Pilot::AxisAlignedBox& bounding_box)
{
    MeshLoaderNode meshes_ = {};
    LoadModelNormal(filename, meshes_);
    
    std::vector<Pilot::MeshVertexDataDefinition> vertexs_;
    std::vector<std::uint16_t>                   indices_;
    RecursiveLoad(meshes_, vertexs_, indices_);

    Pilot::StaticMeshData mesh_data;
    std::uint16_t         stride = sizeof(Pilot::MeshVertexDataDefinition);
    mesh_data.m_vertex_buffer    = std::make_shared<Pilot::BufferData>(vertexs_.size() * stride);
    mesh_data.m_index_buffer     = std::make_shared<Pilot::BufferData>(indices_.size() * sizeof(std::uint16_t));
    bounding_box                 = meshes_.bounding_box_;

    return mesh_data;
}

void ProcessNode(aiNode* node, const aiScene* scene, MeshLoaderNode& meshes_)
{
    for (UINT i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh*         mesh  = scene->mMeshes[node->mMeshes[i]];
        ModelLoaderMesh mesh_ = ProcessMesh(mesh);

        MittTangentsHelper::calc(&mesh_);

        meshes_.meshes_.push_back(mesh_);
        meshes_.bounding_box_.merge(mesh_.bounding_box_);
    }

    meshes_.children_.resize(node->mNumChildren);
    for (UINT i = 0; i < node->mNumChildren; i++)
    {
        ProcessNode(node->mChildren[i], scene, meshes_.children_[i]);
        meshes_.bounding_box_.merge(meshes_.children_[i].bounding_box_);
    }
}

ModelLoaderMesh ProcessMesh(aiMesh* mesh)
{
    // Data to fill
    Pilot::AxisAlignedBox                        bounding_box;
    std::vector<Pilot::MeshVertexDataDefinition> vertices;
    std::vector<std::uint16_t>                   indices;

    // Walk through each of the mesh's vertices
    for (UINT i = 0; i < mesh->mNumVertices; i++)
    {
        Pilot::MeshVertexDataDefinition vertex;

        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        if (mesh->mTextureCoords[0])
        {
            vertex.u = mesh->mTextureCoords[0][i].x;
            vertex.v = mesh->mTextureCoords[0][i].y;
        }

        if (mesh->HasNormals())
        {
            vertex.nx = mesh->mNormals[i].x;
            vertex.ny = mesh->mNormals[i].y;
            vertex.nz = mesh->mNormals[i].z;
        }

        vertices.push_back(vertex);
        bounding_box.merge(Pilot::Vector3(vertex.x, vertex.y, vertex.z));
    }

    for (UINT i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];

        for (UINT j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    return ModelLoaderMesh {bounding_box, vertices, indices};
}

void LoadModelNormal(std::string filename, MeshLoaderNode& meshes_)
{
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(
        filename, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_OptimizeMeshes | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOG_ERROR("ERROR::ASSIMP::{}", importer.GetErrorString());
        return;
    }
    std::string directory = filename.substr(0, filename.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene, meshes_);
}


void MittTangentsHelper::calc(ModelLoaderMesh* mesh_)
{
    SMikkTSpaceInterface iface {};
    iface.m_getNumFaces          = get_num_faces;
    iface.m_getNumVerticesOfFace = get_num_vertices_of_face;
    iface.m_getNormal            = get_normal;
    iface.m_getPosition          = get_position;
    iface.m_getTexCoord          = get_tex_coords;
    iface.m_setTSpaceBasic       = set_tspace_basic;

    SMikkTSpaceContext context {};
    context.m_pInterface = &iface;
    context.m_pUserData  = mesh_;

    genTangSpaceDefault(&context);
}

int MittTangentsHelper::get_num_faces(const SMikkTSpaceContext* context)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    float f_size = (float)working_mesh->vertexs_.size() / 3.f;
    int   i_size = (int)working_mesh->indices_.size() / 3;

    assert((f_size - (float)i_size) == 0.f);

    return i_size;
}

int MittTangentsHelper::get_num_vertices_of_face(const SMikkTSpaceContext* context, const int iFace) { return 3; }

void MittTangentsHelper::get_position(const SMikkTSpaceContext* context,
                                      float*                    outpos,
                                      const int                 iFace,
                                      const int                 iVert)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    auto index  = get_vertex_index(context, iFace, iVert);
    auto vertex = working_mesh->vertexs_[index];

    outpos[0] = vertex.x;
    outpos[1] = vertex.y;
    outpos[2] = vertex.z;
}

void MittTangentsHelper::get_normal(const SMikkTSpaceContext* context,
                                    float*                    outnormal,
                                    const int                 iFace,
                                    const int                 iVert)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    auto index  = get_vertex_index(context, iFace, iVert);
    auto vertex = working_mesh->vertexs_[index];

    outnormal[0] = vertex.nx;
    outnormal[1] = vertex.ny;
    outnormal[2] = vertex.nz;
}

void MittTangentsHelper::get_tex_coords(const SMikkTSpaceContext* context,
                                        float*                    outuv,
                                        const int                 iFace,
                                        const int                 iVert)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    auto index  = get_vertex_index(context, iFace, iVert);
    auto vertex = working_mesh->vertexs_[index];

    outuv[0] = vertex.u;
    outuv[1] = vertex.v;
}

void MittTangentsHelper::set_tspace_basic(const SMikkTSpaceContext* context,
                                    const float*              tangentu,
                                    const float               fSign,
                                    const int                 iFace,
                                    const int                 iVert)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    auto  index  = get_vertex_index(context, iFace, iVert);
    auto* vertex = &working_mesh->vertexs_[index];

    vertex->tx = tangentu[0];
    vertex->ty = tangentu[1];
    vertex->tz = tangentu[2];
    vertex->tw = fSign;
}

int MittTangentsHelper::get_vertex_index(const SMikkTSpaceContext* context, int iFace, int iVert)
{
    ModelLoaderMesh* working_mesh = static_cast<ModelLoaderMesh*>(context->m_pUserData);

    auto face_size = get_num_vertices_of_face(context, iFace);

    auto indices_index = (iFace * face_size) + iVert;

    int index = working_mesh->indices_[indices_index];
    return index;
}






