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

#ifndef SRC_VULKAN_SBT_H_
#define SRC_VULKAN_SBT_H_

#include "src/acceleration_structure.h"
#include "src/vulkan/device.h"
#include "src/vulkan/transfer_buffer.h"

namespace amber {
namespace vulkan {

class SBT {
 public:
  explicit SBT(Device* device);
  ~SBT();

  Result Create(amber::SBT* sbt, VkPipeline pipeline);
  TransferBuffer* getBuffer() { return buffer.get(); }

 private:
  Device* device_ = nullptr;
  std::unique_ptr<TransferBuffer> buffer;
};

}  // namespace vulkan
}  // namespace amber

#endif  // SRC_VULKAN_SBT_H_