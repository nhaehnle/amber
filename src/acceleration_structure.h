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
#ifndef SRC_ACCELERATION_STRUCTURE_H_
#define SRC_ACCELERATION_STRUCTURE_H_

#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "amber/amber.h"
#include "amber/result.h"
#include "amber/value.h"
#include "src/format.h"
#include "src/image.h"

namespace amber {

enum class GeometryType : int8_t {
  kUnknown = 0,
  kTriangle,
  kAABB,
};

class Shader;

class Geometry {
 public:
  Geometry();
  ~Geometry();

  void SetType(GeometryType type) { type_ = type; }
  GeometryType GetType() { return type_; }

  void SetData(std::vector<float>& data) { data_.swap(data); }
  std::vector<float>& GetData() { return data_; }

  size_t getVertexCount(void) const {
    return data_.size() / 3;
  }

  size_t getPrimitiveCount(void) const {
    return IsTriangle() ? (getVertexCount() / 3)
           : IsAABB()   ? (getVertexCount() / 2)
                        : 0;
  }

  bool IsTriangle() const { return type_ == GeometryType::kTriangle; }
  bool IsAABB() const { return type_ == GeometryType::kAABB; }

 private:
  GeometryType type_ = GeometryType::kUnknown;
  std::vector<float> data_;
};

class BLAS {
 public:
  BLAS();
  ~BLAS();

  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const { return name_; }

  void AddGeometry(std::shared_ptr<Geometry> geometry) {
    geometry_.push_back(geometry);
  }
  size_t GetGeometrySize() { return geometry_.size(); }
  std::vector<std::shared_ptr<Geometry>>& GetGeometries() {
    return geometry_;
  }

 private:
  std::string name_;
  std::vector<std::shared_ptr<Geometry>> geometry_;
};

class BLASInstance {
 public:
  BLASInstance();
  ~BLASInstance();

  void SetUsedBLAS(const std::string& name, BLAS* blas) {
    used_blas_name_ = name;
    used_blas_ptr_ = blas;
  }
  std::string GetUsedBLASName() const { return used_blas_name_; }
  BLAS* GetUsedBLASPtr() const { return used_blas_ptr_; }

  void SetTransform(const std::vector<float>& transform) {
    transform_ = transform;
  }
  const float* GetTransform() const { return transform_.data(); }

  void SetInstanceIndex(uint32_t instanceCustomIndex) {
    instanceCustomIndex_ = instanceCustomIndex;
    assert(instanceCustomIndex_ == instanceCustomIndex);
  }
  uint32_t GetInstanceIndex() const { return instanceCustomIndex_; }

  void SetMask(uint32_t mask) {
    mask_ = mask;
    assert(mask_ == mask);
  }
  uint32_t GetMask() const { return mask_; }

  void SetOffset(uint32_t offset) {
    instanceShaderBindingTableRecordOffset_ = offset;
    assert(instanceShaderBindingTableRecordOffset_ == offset);
  }
  uint32_t GetOffset() const { return instanceShaderBindingTableRecordOffset_; }

  void SetFlags(uint32_t flags) {
    flags_ = flags;
    assert(flags_ == flags);
  }
  uint32_t GetFlags() const { return flags_; }

 private:
  std::string used_blas_name_;
  BLAS* used_blas_ptr_;
  std::vector<float> transform_;
  uint32_t instanceCustomIndex_ : 24;
  uint32_t mask_ : 8;
  uint32_t instanceShaderBindingTableRecordOffset_ : 24;
  uint32_t flags_ : 8;
};

class TLAS {
 public:
  TLAS();
  ~TLAS();

  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const { return name_; }

  void AddInstance(std::unique_ptr<BLASInstance> instance) {
    blas_instances_.push_back(std::shared_ptr<BLASInstance>(instance.release()));
  }
  size_t GetInstanceSize() { return blas_instances_.size(); }
  std::shared_ptr<BLASInstance>* GetInstanceData() {
    return blas_instances_.data();
  }
  std::vector<std::shared_ptr<BLASInstance>>& GetInstances() {
    return blas_instances_;
  }

 private:
  std::string name_;
  std::vector<std::shared_ptr<BLASInstance>> blas_instances_;
};

class ShaderGroup {
 public:
  ShaderGroup();
  ~ShaderGroup();

  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const { return name_; }

  void SetGeneralShader(Shader* shader) { generalShader_ = shader; }
  Shader* GetGeneralShader() const { return generalShader_; }

  void SetClosestHitShader(Shader* shader) {
    closestHitShader_ = shader;
  }
  Shader* GetClosestHitShader() const { return closestHitShader_; }

  void SetAnyHitShader(Shader* shader) { anyHitShader_ = shader; }
  Shader* GetAnyHitShader() const { return anyHitShader_; }

  void SetIntersectionShader(Shader* shader) {
    intersectionShader_ = shader;
  }
  Shader* GetIntersectionShader() const { return intersectionShader_; }

  bool IsGeneralGroup() const { return generalShader_ != nullptr; }
  bool IsHitGroup() const {
    return closestHitShader_ != nullptr || anyHitShader_ != nullptr ||
           intersectionShader_ != nullptr;
  }

  std::string name_;
  Shader* generalShader_;
  Shader* closestHitShader_;
  Shader* anyHitShader_;
  Shader* intersectionShader_;
};

class SBTRecord {
 public:
  SBTRecord();
  ~SBTRecord();

  void SetUsedShaderGroupName(const std::string& shader_group_name, uint32_t pipeline_index) {
    used_shader_group_name_ = shader_group_name;
    pipeline_index_ = pipeline_index;
  }
  std::string GetUsedShaderGroupName() const { return used_shader_group_name_; }
  uint32_t GetUsedShaderGroupPipelineIndex() const {
    return pipeline_index_;
  }

  void SetCount(const uint32_t count) { count_ = count; }
  uint32_t GetCount() const { return count_; }

  std::string used_shader_group_name_;
  uint32_t count_;
  uint32_t pipeline_index_;
};

class SBT {
 public:
  SBT();
  ~SBT();

  void SetName(const std::string& name) { name_ = name; }
  std::string GetName() const { return name_; }

  void AddSBTRecord(std::shared_ptr<SBTRecord> record) {
    record_.push_back(record);
  }
  size_t GetSBTRecordCount() { return record_.size(); }
  std::vector<std::shared_ptr<SBTRecord>>& GetSBTRecords() { return record_; }
  uint32_t GetSBTSize() {
    uint32_t size = 0;
    for (auto& x : record_)
      size += x->GetCount();

    return size;
  }

 private:
  std::string name_;
  std::vector<std::shared_ptr<SBTRecord>> record_;
};

}  // namespace amber

#endif  // SRC_ACCELERATION_STRUCTURE_H_
