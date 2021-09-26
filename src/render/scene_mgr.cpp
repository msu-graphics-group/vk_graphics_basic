#include <map>
#include <array>
#include "scene_mgr.h"
#include "vk_utils.h"
#include "vk_buffers.h"
#include "../loader_utils/hydraxml.h"

VkTransformMatrixKHR transformMatrixFromFloat4x4(const LiteMath::float4x4 &m)
{
  VkTransformMatrixKHR transformMatrix;
  for(int i = 0; i < 3; ++i)
  {
    for(int j = 0; j < 4; ++j)
    {
      transformMatrix.matrix[i][j] = m(i, j);
    }
  }

  return transformMatrix;
}

SceneManager::SceneManager(VkDevice a_device, VkPhysicalDevice a_physDevice,
  uint32_t a_transferQId, uint32_t a_graphicsQId, bool debug) : m_device(a_device), m_physDevice(a_physDevice),
                 m_transferQId(a_transferQId), m_graphicsQId(a_graphicsQId), m_debug(debug)
{
  vkGetDeviceQueue(m_device, m_transferQId, 0, &m_transferQ);
  vkGetDeviceQueue(m_device, m_graphicsQId, 0, &m_graphicsQ);
  VkDeviceSize scratchMemSize = 64 * 1024 * 1024;
  m_pCopyHelper = std::make_unique<vk_utils::PingPongCopyHelper>(m_physDevice, m_device, m_transferQ, m_transferQId, scratchMemSize);
  m_pMeshData   = std::make_shared<Mesh8F>();

}

bool SceneManager::LoadSceneXML(const std::string &scenePath, bool transpose)
{
  auto hscene_main = std::make_shared<hydra_xml::HydraScene>();
  auto res         = hscene_main->LoadState(scenePath);

  if(res < 0)
  {
    RUN_TIME_ERROR("LoadSceneXML error");
    return false;
  }

  for(auto loc : hscene_main->MeshFiles())
  {
    auto meshId    = AddMeshFromFile(loc);
    auto instances = hscene_main->GetAllInstancesOfMeshLoc(loc); 
    for(size_t j = 0; j < instances.size(); ++j)
    {
      if(transpose)
        InstanceMesh(meshId, LiteMath::transpose(instances[j]));
      else
        InstanceMesh(meshId, instances[j]);
    }
  }

  LoadGeoDataOnGPU();
  hscene_main = nullptr;

  return true;
}

void SceneManager::LoadSingleTriangle()
{
  std::vector<Vertex> vertices =
  {
    { {  1.0f,  1.0f, 0.0f } },
    { { -1.0f,  1.0f, 0.0f } },
    { {  0.0f, -1.0f, 0.0f } }
  };

  std::vector<uint32_t> indices = { 0, 1, 2 };
  m_totalIndices = static_cast<uint32_t>(indices.size());

  VkDeviceSize vertexBufSize = sizeof(Vertex) * vertices.size();
  VkDeviceSize indexBufSize  = sizeof(uint32_t) * indices.size();
  
  VkMemoryRequirements vertMemReq, idxMemReq; 
  m_geoVertBuf = vk_utils::createBuffer(m_device, vertexBufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &vertMemReq);
  m_geoIdxBuf  = vk_utils::createBuffer(m_device, indexBufSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &idxMemReq);

  size_t pad = vk_utils::getPaddedSize(vertMemReq.size, idxMemReq.alignment);

  VkMemoryAllocateInfo allocateInfo = {};
  allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocateInfo.pNext           = nullptr;
  allocateInfo.allocationSize  = pad + idxMemReq.size;
  allocateInfo.memoryTypeIndex = vk_utils::findMemoryType(vertMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_physDevice);


  VK_CHECK_RESULT(vkAllocateMemory(m_device, &allocateInfo, nullptr, &m_geoMemAlloc));

  VK_CHECK_RESULT(vkBindBufferMemory(m_device, m_geoVertBuf, m_geoMemAlloc, 0));
  VK_CHECK_RESULT(vkBindBufferMemory(m_device, m_geoIdxBuf,  m_geoMemAlloc, pad));
  m_pCopyHelper->UpdateBuffer(m_geoVertBuf, 0, vertices.data(),  vertexBufSize);
  m_pCopyHelper->UpdateBuffer(m_geoIdxBuf,  0, indices.data(), indexBufSize);
}



uint32_t SceneManager::AddMeshFromFile(const std::string& meshPath)
{
  //@TODO: other file formats
  auto data = cmesh::LoadMeshFromVSGF(meshPath.c_str());

  if(data.VerticesNum() == 0)
    RUN_TIME_ERROR(("can't load mesh at " + meshPath).c_str());

  return AddMeshFromData(data);
}

uint32_t SceneManager::AddMeshFromData(cmesh::SimpleMesh &meshData)
{
  assert(meshData.VerticesNum() > 0);
  assert(meshData.IndicesNum() > 0);

  m_pMeshData->Append(meshData);

  MeshInfo info;
  info.m_vertNum = meshData.VerticesNum();
  info.m_indNum  = meshData.IndicesNum();

  info.m_vertexOffset = m_totalVertices;
  info.m_indexOffset  = m_totalIndices;

  info.m_vertexBufOffset = info.m_vertexOffset * m_pMeshData->SingleVertexSize();
  info.m_indexBufOffset  = info.m_indexOffset  * m_pMeshData->SingleIndexSize();

  m_totalVertices += meshData.VerticesNum();
  m_totalIndices  += meshData.IndicesNum();

  m_meshInfos.push_back(info);

  return m_meshInfos.size() - 1;
}

uint32_t SceneManager::InstanceMesh(const uint32_t meshId, const LiteMath::float4x4 &matrix, bool markForRender)
{
  assert(meshId < m_meshInfos.size());

  //@TODO: maybe move
  m_instanceMatrices.push_back(matrix);

  InstanceInfo info;
  info.inst_id       = m_instanceMatrices.size() - 1;
  info.mesh_id       = meshId;
  info.renderMark    = markForRender;
  info.instBufOffset = (m_instanceMatrices.size() - 1) * sizeof(matrix);

  m_instanceInfos.push_back(info);

  return info.inst_id;
}

void SceneManager::MarkInstance(const uint32_t instId)
{
  assert(instId < m_instanceInfos.size());
  m_instanceInfos[instId].renderMark = true;
}

void SceneManager::UnmarkInstance(const uint32_t instId)
{
  assert(instId < m_instanceInfos.size());
  m_instanceInfos[instId].renderMark = false;
}

void SceneManager::LoadGeoDataOnGPU()
{
  VkDeviceSize vertexBufSize = m_pMeshData->VertexDataSize();
  VkDeviceSize indexBufSize  = m_pMeshData->IndexDataSize();
  VkDeviceSize infoBufSize   = m_meshInfos.size() * sizeof(uint32_t) * 2;

  m_geoVertBuf  = vk_utils::createBuffer(m_device, vertexBufSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  m_geoIdxBuf   = vk_utils::createBuffer(m_device, indexBufSize,  VK_BUFFER_USAGE_INDEX_BUFFER_BIT  | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  m_meshInfoBuf = vk_utils::createBuffer(m_device, infoBufSize,   VK_BUFFER_USAGE_TRANSFER_DST_BIT);

  VkMemoryAllocateFlags allocFlags {};

  m_geoMemAlloc = vk_utils::allocateAndBindWithPadding(m_device, m_physDevice, {m_geoVertBuf, m_geoIdxBuf, m_meshInfoBuf}, allocFlags);

  std::vector<LiteMath::uint2> mesh_info_tmp;
  for(const auto& m : m_meshInfos)
  {
    mesh_info_tmp.emplace_back(m.m_indexOffset, m.m_vertexOffset);
  }

  m_pCopyHelper->UpdateBuffer(m_geoVertBuf, 0, m_pMeshData->VertexData(), vertexBufSize);
  m_pCopyHelper->UpdateBuffer(m_geoIdxBuf,  0, m_pMeshData->IndexData(), indexBufSize);
  if(!mesh_info_tmp.empty())
    m_pCopyHelper->UpdateBuffer(m_meshInfoBuf,  0, mesh_info_tmp.data(), mesh_info_tmp.size() * sizeof(mesh_info_tmp[0]));
}

void SceneManager::DrawMarkedInstances()
{

}

void SceneManager::DestroyScene()
{
  if(m_geoVertBuf != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_geoVertBuf, nullptr);
    m_geoVertBuf = VK_NULL_HANDLE;
  }

  if(m_geoIdxBuf != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_geoIdxBuf, nullptr);
    m_geoIdxBuf = VK_NULL_HANDLE;
  }

  if(m_instanceMatricesBuffer != VK_NULL_HANDLE)
  {
    vkDestroyBuffer(m_device, m_instanceMatricesBuffer, nullptr);
    m_instanceMatricesBuffer = VK_NULL_HANDLE;
  }

  if(m_geoMemAlloc != VK_NULL_HANDLE)
  {
    vkFreeMemory(m_device, m_geoMemAlloc, nullptr);
    m_geoMemAlloc = VK_NULL_HANDLE;
  }

  m_pCopyHelper = nullptr;

  m_meshInfos.clear();
  m_pMeshData = nullptr;
  m_instanceInfos.clear();
  m_instanceMatrices.clear();
}