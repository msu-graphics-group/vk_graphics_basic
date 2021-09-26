#ifndef CHIMERA_SCENE_MGR_H
#define CHIMERA_SCENE_MGR_H

#include <vector>

#include <geom/vk_mesh.h>
#include "LiteMath.h"
#include <vk_copy.h>

#include "../resources/shaders/common.h"

struct InstanceInfo
{
  uint32_t inst_id = 0u;
  uint32_t mesh_id = 0u;
  VkDeviceSize instBufOffset = 0u;
  bool renderMark = false;
};

struct SceneManager
{
  SceneManager(VkDevice a_device, VkPhysicalDevice a_physDevice, uint32_t a_transferQId, uint32_t a_graphicsQId,
    bool debug = false);
  ~SceneManager() { DestroyScene(); }

  bool LoadSceneXML(const std::string &scenePath, bool transpose = true);
  void LoadSingleTriangle();

  uint32_t AddMeshFromFile(const std::string& meshPath);
  uint32_t AddMeshFromData(cmesh::SimpleMesh &meshData);

  uint32_t InstanceMesh(uint32_t meshId, const LiteMath::float4x4 &matrix, bool markForRender = true);

  void MarkInstance(uint32_t instId);
  void UnmarkInstance(uint32_t instId);

  void DrawMarkedInstances();

  void DestroyScene();

  VkPipelineVertexInputStateCreateInfo GetPipelineVertexInputStateCreateInfo() { return m_pMeshData->VertexInputLayout();}

  VkBuffer GetVertexBuffer() const { return m_geoVertBuf; }
  VkBuffer GetIndexBuffer()  const { return m_geoIdxBuf; }
  VkBuffer GetMeshInfoBuffer()  const { return m_meshInfoBuf; }
  std::shared_ptr<vk_utils::ICopyEngine> GetCopyHelper() { return  m_pCopyHelper; }

  uint32_t MeshesNum() const {return m_meshInfos.size();}
  uint32_t InstancesNum() const {return m_instanceInfos.size();}

  MeshInfo GetMeshInfo(uint32_t meshId) const {assert(meshId < m_meshInfos.size()); return m_meshInfos[meshId];}
  InstanceInfo GetInstanceInfo(uint32_t instId) const {assert(instId < m_instanceInfos.size()); return m_instanceInfos[instId];}
  LiteMath::float4x4 GetInstanceMatrix(uint32_t instId) const {assert(instId < m_instanceMatrices.size()); return m_instanceMatrices[instId];}

private:
  void LoadGeoDataOnGPU();

  std::vector<MeshInfo> m_meshInfos = {};
  std::shared_ptr<IMeshData> m_pMeshData = nullptr;

  std::vector<InstanceInfo> m_instanceInfos = {};
  std::vector<LiteMath::float4x4> m_instanceMatrices = {};

  uint32_t m_totalVertices = 0u;
  uint32_t m_totalIndices  = 0u;

  VkBuffer m_geoVertBuf = VK_NULL_HANDLE;
  VkBuffer m_geoIdxBuf  = VK_NULL_HANDLE;
  VkBuffer m_meshInfoBuf  = VK_NULL_HANDLE;
  VkBuffer m_instanceMatricesBuffer = VK_NULL_HANDLE;
  VkDeviceMemory m_geoMemAlloc = VK_NULL_HANDLE;

  VkDevice m_device = VK_NULL_HANDLE;
  VkPhysicalDevice m_physDevice = VK_NULL_HANDLE;
  uint32_t m_transferQId = UINT32_MAX;
  VkQueue m_transferQ = VK_NULL_HANDLE;

  uint32_t m_graphicsQId = UINT32_MAX;
  VkQueue m_graphicsQ = VK_NULL_HANDLE;
  std::shared_ptr<vk_utils::ICopyEngine> m_pCopyHelper;

  bool m_debug = false;
  // for debugging
  struct Vertex
  {
    float pos[3];
  };
};

#endif//CHIMERA_SCENE_MGR_H
