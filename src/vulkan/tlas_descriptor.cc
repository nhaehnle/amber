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

#include "src/vulkan/tlas_descriptor.h"

#include "src/vulkan/device.h"
#include "src/vulkan/resource.h"

namespace amber {
namespace vulkan {

TLASDescriptor::TLASDescriptor(amber::TLAS* tlas,
                               DescriptorType type,
                               Device* device,
                               blases_t* blases,
                               tlases_t* tlases,
                               uint32_t desc_set,
                               uint32_t binding)
    : Descriptor(type, device, desc_set, binding),
      blases_(blases),
      tlases_(tlases) {
  assert(blases != nullptr);
  assert(tlases != nullptr);
  AddAmberTLAS(tlas);
}

TLASDescriptor::~TLASDescriptor() = default;

Result TLASDescriptor::CreateResourceIfNeeded() {
  vulkan_tlases_.reserve(amber_tlases_.size());
  for (auto& tlas : amber_tlases_) {
    vulkan_tlases_.emplace_back(MakeUnique<TLAS>(device_));
    amber::TLAS* k = tlas;
    amber::vulkan::TLAS* v = vulkan_tlases_.back().get();
    tlases_->emplace(k, v);
    Result r = vulkan_tlases_.back()->CreateTLAS(tlas, blases_, tlases_);
    if (!r.IsSuccess())
      return r;
  }

  return {};
}

void TLASDescriptor::UpdateDescriptorSetIfNeeded(
    VkDescriptorSet descriptor_set) {
  std::vector<VkAccelerationStructureKHR> as;

  for (auto& t : vulkan_tlases_)
    as.push_back(t->GetVkTLAS());

  VkWriteDescriptorSetAccelerationStructureKHR writeDescriptorTlas;
  writeDescriptorTlas.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  writeDescriptorTlas.pNext = nullptr;
  writeDescriptorTlas.accelerationStructureCount = (uint32_t)as.size();
  writeDescriptorTlas.pAccelerationStructures = as.data();

  VkWriteDescriptorSet write = VkWriteDescriptorSet();
  write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  write.pNext = &writeDescriptorTlas;
  write.dstSet = descriptor_set;
  write.dstBinding = binding_;
  write.dstArrayElement = 0;
  write.descriptorCount = (uint32_t)as.size();
  write.descriptorType = GetVkDescriptorType();

  device_->GetPtrs()->vkUpdateDescriptorSets(device_->GetVkDevice(), 1, &write,
                                             0, nullptr);
}

}  // namespace vulkan
}  // namespace amber
