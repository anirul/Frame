#pragma once

#include <gtest/gtest.h>

#include <memory>

#include "frame/file/ply.h"

namespace test {

class PlyTest : public testing::Test {
   public:
    PlyTest() = default;

   protected:
    std::unique_ptr<frame::file::Ply> ply_;
};

}  // End namespace test.
