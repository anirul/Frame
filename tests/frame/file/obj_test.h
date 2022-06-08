#pragma once

#include <gtest/gtest.h>

#include "frame/file/obj.h"

namespace test {

class ObjTest : public testing::Test {
   public:
    ObjTest() = default;

   protected:
    std::unique_ptr<frame::file::Obj> obj_;
};

}  // End namespace test.
