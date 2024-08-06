#include "scene_mgr.h"
#include "../loader_utils/hydraxml.h"

#include <etna/GlobalContext.hpp>


VkTransformMatrixKHR transformMatrixFromFloat4x4(const LiteMath::float4x4 &m)
{
  VkTransformMatrixKHR transformMatrix;
  for(int i = 0; i < 3; ++i)
    for(int j = 0; j < 4; ++j)
      transformMatrix.matrix[i][j] = m(i, j);

  return transformMatrix;
}

SceneManager::SceneManager()
{
  oneShotCommands = etna::get_context().createOneShotCmdMgr();
  constexpr vk::DeviceSize stagingMemSize = 64 * 1024 * 1024;
  transferHelper = std::make_unique<etna::BlockingTransferHelper>(etna::BlockingTransferHelper::CreateInfo{.stagingSize = stagingMemSize});
  m_pMeshData = std::make_shared<Mesh8F>();
}

bool SceneManager::LoadSceneXML(const std::string &scenePath, bool transpose)
{
  auto hscene_main = std::make_shared<hydra_xml::HydraScene>();
  auto res         = hscene_main->LoadState(scenePath);

  if(res < 0)
  {
    ETNA_PANIC("LoadSceneXML error");
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

  for(auto cam : hscene_main->Cameras())
  {
    m_sceneCameras.push_back(cam);
  }

  LoadGeoDataOnGPU();
  hscene_main = nullptr;

  return true;
}

hydra_xml::Camera SceneManager::GetCamera(uint32_t camId) const
{
  if(camId >= m_sceneCameras.size())
  {
    spdlog::warn("SceneManager: Camera with id = {} was not loaded, using default camera.", camId);

    hydra_xml::Camera res = {};
    res.fov = 60;
    res.nearPlane = 0.1f;
    res.farPlane  = 1000.0f;
    res.pos[0] = 0.0f; res.pos[1] = 0.0f; res.pos[2] = 15.0f;
    res.up[0] = 0.0f; res.up[1] = 1.0f; res.up[2] = 0.0f;
    res.lookAt[0] = 0.0f; res.lookAt[1] = 0.0f; res.lookAt[2] = 0.0f;

    return res;
  }

  return m_sceneCameras[camId];
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

  geoVertBuf =  etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = vertexBufSize,
    .bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "geoVertBuf",
  });

  geoIdxBuf =  etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = indexBufSize,
    .bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "geoIdxBuf",
  });

  transferHelper->uploadBuffer<Vertex>(*oneShotCommands, geoVertBuf, 0, vertices);
  transferHelper->uploadBuffer<uint32_t>(*oneShotCommands, geoIdxBuf, 0, indices);
}

uint32_t SceneManager::AddMeshFromFile(const std::string& meshPath)
{
  //@TODO: other file formats
  auto data = cmesh::LoadMeshFromVSGF(meshPath.c_str());

  ETNA_ASSERTF(data.VerticesNum() != 0, "can't load mesh at {}", meshPath);

  return AddMeshFromData(data);
}

uint32_t SceneManager::AddMeshFromData(cmesh::SimpleMesh &meshData)
{
  assert(meshData.VerticesNum() > 0);
  assert(meshData.IndicesNum() > 0);

  m_pMeshData->Append(meshData);

  MeshInfo info;
  info.m_vertNum = (uint32_t)meshData.VerticesNum();
  info.m_indNum  = (uint32_t)meshData.IndicesNum();

  info.m_vertexOffset = m_totalVertices;
  info.m_indexOffset  = m_totalIndices;

  info.m_vertexBufOffset = info.m_vertexOffset * m_pMeshData->SingleVertexSize();
  info.m_indexBufOffset  = info.m_indexOffset  * m_pMeshData->SingleIndexSize();

  m_totalVertices += (uint32_t)meshData.VerticesNum();
  m_totalIndices  += (uint32_t)meshData.IndicesNum();

  m_meshInfos.push_back(info);
  LiteMath::Box4f meshBox;
  for (uint32_t i = 0; i < meshData.VerticesNum(); ++i) {
    meshBox.include(reinterpret_cast<LiteMath::float4*>(meshData.vPos4f.data())[i]);
  }
  m_meshBboxes.push_back(meshBox);

  return (uint32_t)m_meshInfos.size() - 1;
}

uint32_t SceneManager::InstanceMesh(const uint32_t meshId, const LiteMath::float4x4 &matrix, bool markForRender)
{
  assert(meshId < m_meshInfos.size());

  //@TODO: maybe move
  m_instanceMatrices.push_back(matrix);

  InstanceInfo info;
  info.inst_id       = (uint32_t)m_instanceMatrices.size() - 1;
  info.mesh_id       = meshId;
  info.renderMark    = markForRender;
  info.instBufOffset = (m_instanceMatrices.size() - 1) * sizeof(matrix);

  m_instanceInfos.push_back(info);

  LiteMath::Box4f instBox;
  for (uint32_t i = 0; i < 8; ++i) {
    LiteMath::float4 corner = LiteMath::float4(
      (i & 1) == 0 ? m_meshBboxes[meshId].boxMin.x : m_meshBboxes[meshId].boxMax.x,
      (i & 2) == 0 ? m_meshBboxes[meshId].boxMin.y : m_meshBboxes[meshId].boxMax.y,
      (i & 4) == 0 ? m_meshBboxes[meshId].boxMin.z : m_meshBboxes[meshId].boxMax.z,
      1
    );
    instBox.include(matrix * corner);
  }
  sceneBbox.include(instBox);
  m_instanceBboxes.push_back(instBox);

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

  std::vector<LiteMath::uint2> mesh_info_tmp;
  for(const auto& m : m_meshInfos)
    mesh_info_tmp.emplace_back(m.m_indexOffset, m.m_vertexOffset);


  geoVertBuf =  etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = vertexBufSize,
    .bufferUsage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "geoVertBuf",
  });

  geoIdxBuf =  etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = indexBufSize,
    .bufferUsage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "geoIdxBuf",
  });

  meshInfoBuf =  etna::get_context().createBuffer(etna::Buffer::CreateInfo{
    .size = infoBufSize,
    .bufferUsage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eUniformBuffer,
    .memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY,
    .name = "meshInfoBuf",
  });

  transferHelper->uploadBuffer(*oneShotCommands, geoVertBuf, 0, {reinterpret_cast<std::byte*>(m_pMeshData->VertexData()), vertexBufSize});
  transferHelper->uploadBuffer(*oneShotCommands, geoIdxBuf, 0, {reinterpret_cast<std::byte*>(m_pMeshData->IndexData()), indexBufSize});

  transferHelper->uploadBuffer<LiteMath::uint2>(*oneShotCommands, meshInfoBuf, 0, mesh_info_tmp);
}

etna::VertexByteStreamFormatDescription SceneManager::GetVertexStreamDescription()
{
  // legacy vk-utils code always returns a single binding
  // reimplement meshes in etna ASAP
  auto legacyDescription = m_pMeshData->VertexInputLayout();
  etna::VertexByteStreamFormatDescription result;
  result.stride = legacyDescription.pVertexBindingDescriptions->stride;
  result.attributes.reserve(legacyDescription.vertexAttributeDescriptionCount);
  for (uint32_t i = 0; i < legacyDescription.vertexAttributeDescriptionCount; ++i)
  {
    // NOTE: fragile code, we rely on the fact that vk-utils places attributes for
    // location i in ith slot.
    auto& legacyAttr = legacyDescription.pVertexAttributeDescriptions[i];
    result.attributes.push_back(
      etna::VertexByteStreamFormatDescription::Attribute
      {
        .format = static_cast<vk::Format>(legacyAttr.format),
        .offset = legacyAttr.offset
      });
  }

  return result;
}

