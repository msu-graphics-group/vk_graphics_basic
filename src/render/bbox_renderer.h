#ifndef BBOX_RENDERER_H
#define BBOX_RENDERER_H

#include <vector>
#include <etna/Vulkan.hpp>
#include <etna/Image.hpp>

#include <LiteMath.h>

#include "etna/GlobalContext.hpp"
#include "etna/GraphicsPipeline.hpp"
#include "etna/DescriptorSet.hpp"
#include "etna/Buffer.hpp"


class BboxRenderer
{
public:
  struct CreateInfo
  {
    bool drawInstanced  = false;
    vk::Extent2D extent = {};
    vk::Format colorFormat = {};
    vk::Format depthFormat = {};
  };

  BboxRenderer(CreateInfo info);
  ~BboxRenderer() {}

  void SetBoxes(const LiteMath::Box4f *boxes, size_t cnt);
  inline void SetBoxes(const std::vector<LiteMath::Box4f> &boxes) { SetBoxes(boxes.data(), boxes.size()); }

  void RecordCommands(vk::CommandBuffer cmdBuff, 
    vk::Image targetImage, vk::ImageView targetImageView, 
    const etna::Image &depthImage, const LiteMath::float4x4 &mViewProj);

private:
  struct PushConst2M
  {
    LiteMath::float4x4 mViewProj;
    LiteMath::float4x4 mInst;
  };

  std::vector<LiteMath::float4x4> m_instances = {};

  bool m_drawInstanced  = false;
  vk::Extent2D m_extent = {};

  etna::GlobalContext *m_context    = nullptr;

  etna::ShaderProgramId m_programId = (etna::ShaderProgramId)(-1);

  etna::GraphicsPipeline m_pipeline = {};
  etna::Buffer m_vertexBuffer       = {};
  etna::Buffer m_indexBuffer        = {};
  etna::Buffer m_boxesInstBuffer    = {};

  static constexpr float s_boxEps = 0.005f;
  static const LiteMath::float4 s_boxVert[8];
  static const uint32_t s_boxInd[12*2]; // for Line list
};

#endif // BBOX_RENDERER_H
