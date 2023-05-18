#pragma once

#include <gmock/gmock.h>

#include "frame/plugin_interface.h"

namespace test {

class PluginMock : public frame::PluginInterface {
 public:
  MOCK_METHOD(void, Startup, (glm::uvec2), (override));
  MOCK_METHOD(bool, PollEvent, (void*), (override));
  MOCK_METHOD(void, PreRender,
              (frame::UniformInterface&, frame::DeviceInterface&,
               frame::StaticMeshInterface&, frame::MaterialInterface&),
              (override));
  MOCK_METHOD(bool, Update, (frame::DeviceInterface&, double), (override));
  MOCK_METHOD(void, End, (), (override));
  MOCK_METHOD(std::string, GetName, (), (const, override));
  MOCK_METHOD(void, SetName, (const std::string&), (override));
};

}  // End namespace test.
