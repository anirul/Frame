#include "frame/device_interface.h"
#include "frame/input_interface.h"

namespace frame::gui
{

/**
 * @class InputArcball
 * @brief This is where you can handle the input from keyboard and mouse to
 *        the app. See the input interface class for more info.
 */
class InputArcball : public InputInterface
{
  public:
    /**
     * @brief Constructor take arguments for the input.
     * @param device: A pointer to the device.
     * @param move_multiplication: Factor by which the keys are multiply.
     * @param rotation_multiplication: Factor to change the mouse speed when
     *        in movement mode.
     */
    InputArcball(
        DeviceInterface &device,
        glm::vec3 pivot,
        float move_multiplication,
        float zoom_multiplication)
        : device_(device), move_multiplication_(move_multiplication),
          zoom_multiplication_(zoom_multiplication),
          pivot_(glm::vec4(pivot, 1.0f))
    {
    }
    /**
     * @brief Virtual destructor (default).
     */
    virtual ~InputArcball() = default;
    /**
     * @brief Override from input interface.
     */
    bool KeyPressed(char key, double dt) override;
    bool KeyReleased(char key, double dt) override;
    bool MouseMoved(glm::vec2 position, glm::vec2 relative, double dt) override;
    bool MousePressed(char button, double dt) override;
    bool MouseReleased(char button, double dt) override;
    bool WheelMoved(float relative, double dt) override;
    void NextFrame() override;

  private:
    DeviceInterface &device_;
    bool mouse_active_ = false;
    float move_multiplication_ = 1.0f;
    float zoom_multiplication_ = 1.0f;
    glm::uvec2 size_ = {0, 0};
    glm::uvec2 previous_location_ = {0, 0};
    glm::vec4 pivot_ = {0.0f, 0.0f, 0.0f, 1.0f};
};

} // End namespace frame::gui.
