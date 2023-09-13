#include "frame/device_interface.h"
#include "frame/input_interface.h"

namespace frame::gui
{

/**
 * @class InputWasdMouse
 * @brief This is where you can handle the input from keyboard and mouse to
 *        the app (with wheel support).
 *
 * See the input interface class for more info.
 */
class InputWasdMouse : public InputInterface
{
  public:
    /**
     * @brief Constructor take arguments for the input.
     * @param device: A pointer to the device.
     * @param move_multiplication: Factor by which the keys are multiply.
     * @param rotation_multiplication: Factor to change the mouse speed when
     *        in movement mode.
     * @param translation_multiplication: Factor to change the mouse speed
     *        when in translation mode.
     * @param wheel_multiplication: Factor to change the mouse wheel speed.
     */
    InputWasdMouse(
        DeviceInterface &device,
        float move_multiplication,
        float rotation_multiplication,
        float translation_multiplication,
        float wheel_multiplication)
        : device_(device), move_multiplication_(move_multiplication),
          rotation_multiplication_(rotation_multiplication),
          wheel_multiplication_{wheel_multiplication}
    {
    }
    /**
     * @brief Virtual destructor (default).
     */
    virtual ~InputWasdMouse() = default;
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
    bool shift_ = false;
    char mouse_button_pressed_ = 0;
    float move_multiplication_ = 20.0f;
    float rotation_multiplication_ = 1.0f;
    float translation_multiplication_ = 10.0f;
    float wheel_multiplication_ = 160.0f;
    float move_shift_ = 20.0f;
};

} // End namespace frame::gui.
