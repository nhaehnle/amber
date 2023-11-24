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

#include "src/vulkan/blas.h"

namespace amber {
namespace vulkan {

inline VkDeviceSize align(VkDeviceSize v, VkDeviceSize a) {
  return (v + a - 1) & ~(a - 1);
}

BLAS::BLAS(Device* device) : device_(device) {}

Result BLAS::CreateBLAS(amber::BLAS* blas) {
  if (blas_ != VK_NULL_HANDLE)
    return Result("Cannot recreate acceleration structure");

  std::vector<std::shared_ptr<Geometry>>& geometries = blas->GetGeometries();
  std::vector <VkDeviceSize> vertexBufferOffsets;
  VkDeviceSize vertexBufferSize = 0;

  VkDeviceOrHostAddressConstKHR constNullPtr = {};
  VkDeviceOrHostAddressKHR nullPtr = {};

  accelerationStructureGeometriesKHR_.resize(geometries.size());
  accelerationStructureBuildRangeInfoKHR_.resize(geometries.size());
  maxPrimitiveCounts_.resize(geometries.size());
  vertexBufferOffsets.resize(geometries.size());

  for (size_t geometryNdx = 0; geometryNdx < geometries.size(); ++geometryNdx) {
    const std::shared_ptr<Geometry>&        geometryData = geometries[geometryNdx];
    VkDeviceOrHostAddressConstKHR           vertexData = {};
    VkAccelerationStructureGeometryDataKHR  geometry;
    VkGeometryTypeKHR                       geometryType = VK_GEOMETRY_TYPE_MAX_ENUM_KHR;

    if (geometryData->IsTriangle()) {
      VkAccelerationStructureGeometryTrianglesDataKHR   accelerationStructureGeometryTrianglesDataKHR =
      {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,   //  VkStructureType                 sType;
        nullptr,                                                                //  const void*                     pNext;
        VK_FORMAT_R32G32B32_SFLOAT,                                             //  VkFormat                        vertexFormat;
        vertexData,                                                             //  VkDeviceOrHostAddressConstKHR   vertexData;
        3 * sizeof(float),                                                      //  VkDeviceSize                    vertexStride;
        static_cast<uint32_t>(geometryData->getVertexCount()),                  //  uint32_t                        maxVertex;
        VK_INDEX_TYPE_NONE_KHR,                                                 //  VkIndexType                     indexType;
        constNullPtr,                                                           //  VkDeviceOrHostAddressConstKHR   indexData;
        constNullPtr,                                                           //  VkDeviceOrHostAddressConstKHR   transformData;
      };

      geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
      geometry.triangles = accelerationStructureGeometryTrianglesDataKHR;
    }
    else if (geometryData->IsAABB()) {
      const VkAccelerationStructureGeometryAabbsDataKHR     accelerationStructureGeometryAabbsDataKHR =
      {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,   //  VkStructureType                 sType;
        nullptr,                                                            //  const void*                     pNext;
        vertexData,                                                         //  VkDeviceOrHostAddressConstKHR   data;
        sizeof(VkAabbPositionsKHR)                                          //  VkDeviceSize                    stride;
      };

      geometryType = VK_GEOMETRY_TYPE_AABBS_KHR;
      geometry.aabbs = accelerationStructureGeometryAabbsDataKHR;
    }
    else {
      assert(false);
    }

    const VkAccelerationStructureGeometryKHR                accelerationStructureGeometryKHR =
    {
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,    //  VkStructureType                         sType;
      nullptr,                                                  //  const void*                             pNext;
      geometryType,                                             //  VkGeometryTypeKHR                       geometryType;
      geometry,                                                 //  VkAccelerationStructureGeometryDataKHR  geometry;
      0u,                                                       //  VkGeometryFlagsKHR                      flags;
    };
    const VkAccelerationStructureBuildRangeInfoKHR          accelerationStructureBuildRangeInfosKHR =
    {
      (uint32_t)geometryData->getPrimitiveCount(),  //  uint32_t    primitiveCount;
      0,                                            //  uint32_t    primitiveOffset;
      0,                                            //  uint32_t    firstVertex;
      0                                             //  uint32_t    firstTransform;
    };

    accelerationStructureGeometriesKHR_[geometryNdx]        = accelerationStructureGeometryKHR;
    accelerationStructureBuildRangeInfoKHR_[geometryNdx]    = accelerationStructureBuildRangeInfosKHR;
    maxPrimitiveCounts_[geometryNdx]                        = accelerationStructureBuildRangeInfosKHR.primitiveCount;
    vertexBufferOffsets[geometryNdx]                        = vertexBufferSize;
    size_t s1 = sizeof(
        *geometryData->GetData().data());
    vertexBufferSize += align(geometryData->GetData().size() * s1, 8);
  }

  const VkAccelerationStructureGeometryKHR*     accelerationStructureGeometriesKHRPointer   = accelerationStructureGeometriesKHR_.data();
  accelerationStructureBuildGeometryInfoKHR_    =
  {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,   //  VkStructureType                                     sType;
    nullptr,                                                            //  const void*                                         pNext;
    VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,                    //  VkAccelerationStructureTypeKHR                      type;
    0u,                                                                 //  VkBuildAccelerationStructureFlagsKHR                flags;
    VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,                     //  VkBuildAccelerationStructureModeKHR                 mode;
    nullptr,                                                            //  VkAccelerationStructureKHR                          srcAccelerationStructure;
    nullptr,                                                            //  VkAccelerationStructureKHR                          dstAccelerationStructure;
    static_cast<uint32_t>(accelerationStructureGeometriesKHR_.size()),  //  uint32_t                                            geometryCount;
    accelerationStructureGeometriesKHRPointer,                          //  const VkAccelerationStructureGeometryKHR*           pGeometries;
    nullptr,                                                            //  const VkAccelerationStructureGeometryKHR* const*    ppGeometries;
    nullPtr,                                                            //  VkDeviceOrHostAddressKHR                            scratchData;
  };
  VkAccelerationStructureBuildSizesInfoKHR sizeInfo =
  {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,  //  VkStructureType sType;
    nullptr,                                                        //  const void*     pNext;
    0,                                                              //  VkDeviceSize    accelerationStructureSize;
    0,                                                              //  VkDeviceSize    updateScratchSize;
    0                                                               //  VkDeviceSize    buildScratchSize;
  };

  device_->GetPtrs()->vkGetAccelerationStructureBuildSizesKHR(
      device_->GetVkDevice(), VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &accelerationStructureBuildGeometryInfoKHR_, maxPrimitiveCounts_.data(),
      &sizeInfo);

  const uint32_t accelerationStructureSize =
      (uint32_t)sizeInfo.accelerationStructureSize;

  buffer = MakeUnique<TransferBuffer>(
      device_, accelerationStructureSize, nullptr);
  buffer->AddUsageFlags(VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
                        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
  buffer->AddAllocateFlags(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
  buffer->Initialize();

  const VkAccelerationStructureCreateInfoKHR    accelerationStructureCreateInfoKHR
  {
    VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,   //  VkStructureType                         sType;
    nullptr,                                                    //  const void*                             pNext;
    0,                                                          //  VkAccelerationStructureCreateFlagsKHR   createFlags;
    buffer->GetVkBuffer(),                                      //  VkBuffer                                buffer;
    0,                                                          //  VkDeviceSize                            offset;
    accelerationStructureSize,                                  //  VkDeviceSize                            size;
    VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,            //  VkAccelerationStructureTypeKHR          type;
    0                                                           //  VkDeviceAddress                         deviceAddress;
  };

  if (device_->GetPtrs()->vkCreateAccelerationStructureKHR(
          device_->GetVkDevice(), &accelerationStructureCreateInfoKHR, nullptr,
          &blas_) != VK_SUCCESS)
    return Result("Vulkan::Calling vkCreateAccelerationStructureKHR failed");

  accelerationStructureBuildGeometryInfoKHR_.dstAccelerationStructure = blas_;

  if (sizeInfo.buildScratchSize > 0) {
    scratch_buffer_ =
        MakeUnique<TransferBuffer>(device_, (uint32_t)sizeInfo.buildScratchSize, nullptr);
    scratch_buffer_->AddUsageFlags(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                   VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    scratch_buffer_->AddAllocateFlags(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    scratch_buffer_->Initialize();

    accelerationStructureBuildGeometryInfoKHR_.scratchData.deviceAddress =
        scratch_buffer_->getBufferDeviceAddress();
  }

  if (vertexBufferSize > 0) {
    vertex_buffer_ = MakeUnique<TransferBuffer>(
        device_, (uint32_t)vertexBufferSize, nullptr);
    vertex_buffer_->AddUsageFlags(
        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
    vertex_buffer_->AddAllocateFlags(VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
    vertex_buffer_->Initialize();

    for (size_t geometryNdx = 0; geometryNdx < geometries.size();++geometryNdx) {
      VkDeviceOrHostAddressConstKHR p;

      p.deviceAddress = vertex_buffer_.get()->getBufferDeviceAddress() +
                        vertexBufferOffsets[geometryNdx];

      if (geometries[geometryNdx]->IsTriangle()) {
        accelerationStructureGeometriesKHR_[geometryNdx].geometry.triangles.vertexData = p;
      } else if (geometries[geometryNdx]->IsAABB()) {
        accelerationStructureGeometriesKHR_[geometryNdx].geometry.aabbs.data = p;
      } else {
        assert(false);
      }
    }
  }

  return {};
}

Result BLAS::BuildBLAS(VkCommandBuffer cmdBuffer) {
  if (blas_ == VK_NULL_HANDLE)
    return Result("Acceleration structure should be created first");
  if (built)
    return {};

  VkAccelerationStructureBuildRangeInfoKHR*
      accelerationStructureBuildRangeInfoKHRPtr =
          accelerationStructureBuildRangeInfoKHR_.data();

  device_->GetPtrs()->vkCmdBuildAccelerationStructuresKHR(
    cmdBuffer, 1, &accelerationStructureBuildGeometryInfoKHR_,
    &accelerationStructureBuildRangeInfoKHRPtr);

  const VkAccessFlags accessMasks =
      VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR |
      VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  const VkMemoryBarrier memBarrier{
      VK_STRUCTURE_TYPE_MEMORY_BARRIER,
      nullptr,
      accessMasks,
      accessMasks,
  };

  device_->GetPtrs()->vkCmdPipelineBarrier(
      cmdBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
      VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 1, &memBarrier, 0, nullptr, 0, nullptr);

  built = true;

  return {};
}

VkDeviceAddress BLAS::getVkBLASDeviceAddress() {
  VkAccelerationStructureDeviceAddressInfoKHR info = {
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,  // VkStructureType             sType;
      nullptr,                                                           // const void*                 pNext;
      blas_                                                              // VkAccelerationStructureKHR  accelerationStructure;
  };

  assert(blas_ != VK_NULL_HANDLE);

  VkDeviceAddress result =
      device_->GetPtrs()->vkGetAccelerationStructureDeviceAddressKHR(
          device_->GetVkDevice(), &info);

  return result;
}


BLAS::~BLAS() {
  if (blas_ != VK_NULL_HANDLE) {
    device_->GetPtrs()->vkDestroyAccelerationStructureKHR(
        device_->GetVkDevice(), blas_, nullptr);
  }
}

}  // namespace vulkan
}  // namespace amber
