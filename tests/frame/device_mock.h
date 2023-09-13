#pragma once

#include <gmock/gmock.h>

#include "frame/device_interface.h"

namespace test
{

class DeviceMock : public frame::DeviceInterface
{
  public:
    MOCK_METHOD(void, Clear, ((const glm::vec4 &)), (const, override));
    MOCK_METHOD(
        void,
        Startup,
        ((std::unique_ptr<frame::LevelInterface> &&)),
        (override));
    MOCK_METHOD(
        void,
        AddPlugin,
        ((std::unique_ptr<frame::PluginInterface> &&)),
        (override));
    MOCK_METHOD(
        std::vector<frame::PluginInterface *>, GetPluginPtrs, (), (override));
    MOCK_METHOD(
        std::vector<std::string>, GetPluginNames, (), (const, override));
    MOCK_METHOD(void, RemovePluginByName, ((const std::string &)), (override));
    MOCK_METHOD(void, Display, ((double)), (override));
    MOCK_METHOD(void, Cleanup, (), (override));
    MOCK_METHOD(
        void, Resize, ((std::pair<std::uint32_t, std::uint32_t>)), (override));
    MOCK_METHOD(frame::LevelInterface *, GetLevel, (), (override));
    MOCK_METHOD(void *, GetDeviceContext, (), (const, override));
    MOCK_METHOD(void, ScreenShot, ((const std::string &)), (const, override));
    MOCK_METHOD(frame::DeviceEnum, GetDeviceEnum, (), (const, override));
    MOCK_METHOD(
        std::unique_ptr<frame::BufferInterface>,
        CreatePointBuffer,
        ((std::vector<float> &&)),
        (override));
    MOCK_METHOD(
        std::unique_ptr<frame::BufferInterface>,
        CreateIndexBuffer,
        ((std::vector<std::uint32_t> &&)),
        (override));
    MOCK_METHOD(
        std::unique_ptr<frame::StaticMeshInterface>,
        CreateStaticMesh,
        ((const frame::StaticMeshParameter &)),
        (override));
    MOCK_METHOD(
        std::unique_ptr<frame::TextureInterface>,
        CreateTexture,
        ((const frame::TextureParameter &)),
        (override));
};

} // End namespace test.
