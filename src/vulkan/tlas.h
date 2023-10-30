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

#ifndef SRC_VULKAN_TLAS_H_
#define SRC_VULKAN_TLAS_H_

#include "src/acceleration_structure.h"
#include "src/vulkan/device.h"
#include "src/vulkan/transfer_buffer.h"

namespace amber {
namespace vulkan {

class TLAS {
 public:
  explicit TLAS(Device* device);
  ~TLAS();

  Result CreateTLAS(amber::TLAS* tlas, blases_t* blases, tlases_t* tlases);
  Result BuildTLAS(VkCommandBuffer cmdBuffer);
  VkAccelerationStructureKHR GetVkTLAS() { return tlas_; }

 private:
  Device* device_ = nullptr;
  VkAccelerationStructureKHR tlas_ = VK_NULL_HANDLE;
  bool built = false;
  std::unique_ptr<TransferBuffer> buffer;
  std::unique_ptr<TransferBuffer> scratch_buffer_;
  std::unique_ptr<TransferBuffer> instance_buffer_;
  uint32_t instances_count_ = 0;
  VkAccelerationStructureGeometryKHR accelerationStructureGeometryKHR_;
  VkAccelerationStructureBuildGeometryInfoKHR
      accelerationStructureBuildGeometryInfoKHR_;
};

}  // namespace vulkan
}  // namespace amber

#endif  // SRC_VULKAN_TLAS_H_
