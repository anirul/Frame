#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "frame/buffer_interface.h"

namespace frame::vulkan
{

class Buffer : public frame::BufferInterface
{
  public:
    Buffer() = default;
    ~Buffer() override = default;

    void Copy(std::size_t size, const void* data = nullptr) const override;
    void Copy(const std::vector<float>& vector) const override;
    void Copy(const std::vector<std::uint32_t>& vector) const override;
    void Copy(const std::vector<std::uint8_t>& vector) const override;
    void Clear() const override;
    std::size_t GetSize() const override;
    void BindBase(int /*binding*/) const override
    {
        // No-op for the CPU backed implementation.
    }
    const std::vector<std::uint8_t>& GetRawData() const
    {
        return data_;
    }

    std::string GetName() const override
    {
        return name_;
    }

    void SetName(const std::string& name) override
    {
        name_ = name;
    }

  private:
    static void CopyBytes(
        const void* data, std::size_t size, std::vector<std::uint8_t>& dst);

    mutable std::vector<std::uint8_t> data_ = {};
    std::string name_ = {};
};

std::unique_ptr<frame::BufferInterface> CreatePointBuffer(
    std::vector<float>&& vector);
std::unique_ptr<frame::BufferInterface> CreateIndexBuffer(
    std::vector<std::uint32_t>&& vector);

} // namespace frame::vulkan
