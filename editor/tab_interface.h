#pragma once

#include "frame/level_interface.h"
#include "frame/name_interface.h"

namespace frame::gui {

class TabInterface : public NameInterface {
  public:
    explicit TabInterface(std::string name) : name_(std::move(name)) {}
    ~TabInterface() override = default;

    std::string GetName() const override { return name_; }
    void SetName(const std::string& name) override { name_ = name; }

    virtual void Draw(LevelInterface& level) = 0;

  private:
    std::string name_;
};

} // namespace frame::gui
