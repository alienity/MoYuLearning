#ifndef D3D12_INCLUDED
#define D3D12_INCLUDED

#ifdef _CPP_MACRO_
#define uint glm::uint
#define uint2 glm::uvec2
#endif


// Using uint2 version of D3D12_GPU_VIRTUAL_ADDRESS because command signature arguments needs to be 4 byte aligned
// if i use uint64_t, it breaks alignment rules
typedef uint2 D3D12_GPU_VIRTUAL_ADDRESS;
// typedef uint64_t D3D12_GPU_VIRTUAL_ADDRESS;

struct D3D12_DRAW_ARGUMENTS
{
    uint VertexCountPerInstance;
    uint InstanceCount;
    uint StartVertexLocation;
    uint StartInstanceLocation;
};

struct D3D12_DRAW_INDEXED_ARGUMENTS
{
    uint IndexCountPerInstance;
    uint InstanceCount;
    uint StartIndexLocation;
    int	 BaseVertexLocation;
    uint StartInstanceLocation;
};

struct D3D12_DISPATCH_ARGUMENTS
{
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
};

struct D3D12_DISPATCH_MESH_ARGUMENTS
{
    uint ThreadGroupCountX;
    uint ThreadGroupCountY;
    uint ThreadGroupCountZ;
};

struct D3D12_VERTEX_BUFFER_VIEW
{
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    uint					  SizeInBytes;
    uint					  StrideInBytes;
};

struct D3D12_INDEX_BUFFER_VIEW
{
    D3D12_GPU_VIRTUAL_ADDRESS BufferLocation;
    uint					  SizeInBytes;
    uint					  Format;
};

#ifdef _CPP_MACRO_
#undef uint
#undef uint2
#endif

#endif // D3D12_INCLUDED

