// Copyright (C) 2023 Advanced Micro Devices, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef SRC_VULKAN_RAYTRACING_PIPELINE_H_
#define SRC_VULKAN_RAYTRACING_PIPELINE_H_

#include <vector>

#include "amber/result.h"
#include "amber/vulkan_header.h"
#include "src/vulkan/pipeline.h"

namespace amber {
namespace vulkan {

/// Pipepline to handle compute commands.
class RayTracingPipeline : public Pipeline {
 public:
  RayTracingPipeline(
      Device* device,
      blases_t* blases,
      tlases_t* tlases,
      uint32_t fence_timeout_ms,
      const std::vector<VkPipelineShaderStageCreateInfo>& shader_stage_info);
  ~RayTracingPipeline() override;

  Result AddTLASDescriptor(const TLASCommand* cmd);

  Result Initialize(CommandPool* pool,
                    std::vector<VkRayTracingShaderGroupCreateInfoKHR>&
                        shader_group_create_info);

  Result getVulkanSBTRegion(VkPipeline pipeline,
                            amber::SBT* aSBT,
                            VkStridedDeviceAddressRegionKHR* region);

  Result TraceRays(amber::SBT* rSBT,
                   amber::SBT* mSBT,
                   amber::SBT* hSBT,
                   amber::SBT* cSBT,
                   uint32_t x,
                   uint32_t y,
                   uint32_t z);

  virtual blases_t* GetBlases() { return blases_; }
  virtual tlases_t* GetTlases() { return tlases_; }

 private:
  Result CreateVkRayTracingPipeline(const VkPipelineLayout& pipeline_layout,
                                    VkPipeline* pipeline);

  std::vector<VkRayTracingShaderGroupCreateInfoKHR> shader_group_create_info_;
  blases_t* blases_;
  tlases_t* tlases_;
  sbts_t sbtses_;
  std::vector<std::unique_ptr<amber::vulkan::SBT>> sbts_;
};

}  // namespace vulkan
}  // namespace amber

#endif  // SRC_VULKAN_RAYTRACING_PIPELINE_H_