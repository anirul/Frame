#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

namespace frame
{

/**
 * @class InputInterface
 * @brief This is the interface for input.
 */
struct InputInterface
{
    //! @brief Virtual destructor.
    virtual ~InputInterface() = default;
    /**
     * @brief Keyboard interface in case of pressed on a key.
     * @param key: Key of the keyboard that was pressed.
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool KeyPressed(char key, double dt) = 0;
    /**
     * @brief Keyboard interface in case of release of a key.
     * @param key: Key of the keyboard that was pressed.
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool KeyReleased(char key, double dt) = 0;
    /**
     * @brief Mouse moved in relative positions.
     * @param position: Relative position of the mouse (from last).
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool MouseMoved(
        glm::vec2 position, glm::vec2 relative, double dt) = 0;
    /**
     * @brief Mouse pressed (used for mouse buttons).
     * @param button: Button pressed on the mouse.
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool MousePressed(char button, double dt) = 0;
    /**
     * @brief Mouse released (used for mouse buttons).
     * @param button: Button released on the mouse.
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool MouseReleased(char button, double dt) = 0;
    /**
     * @brief Mouse moved in relative positions.
     * @param relative: Relative position of the wheel (from last).
     * @param dt: Delta time from the beginning of the software in seconds.
     * @return True if processed false if not.
     */
    virtual bool WheelMoved(float relative, double dt) = 0;
    //! @brief Validate the next frame.
    virtual void NextFrame() = 0;
};

} // End namespace frame.
