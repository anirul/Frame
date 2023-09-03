#pragma once

#include <glm/glm.hpp>
#include <string>

#include "frame/api.h"
#include "frame/gui/draw_gui_interface.h"

namespace frame::gui {

	/**
	 * @class WindowResolution
	 * @brief Select to change resolution window.
	 */
	class WindowResolution : public GuiWindowInterface {
	public:
		/**
		 * @brief Default constructor.
		 * @param name: The name of the window.
		 * @param size: The initial size of the window.
		 */
		WindowResolution(const std::string& name, glm::uvec2 size,
			glm::uvec2 border_less_size, glm::vec2 pixel_per_inch);
		//! @brief Virtual destructor.
		virtual ~WindowResolution() = default;

	public:
		//! @brief Draw callback setting.
		bool DrawCallback() override;
		/**
		 * @brief Get the name of the window.
		 * @return The name of the window.
		 */
		std::string GetName() const override;
		/**
		 * @brief Set the name of the window.
		 * @param name: The name of the window.
		 */
		void SetName(const std::string& name) override;
		/**
		 * @brief Get the window size.
		 * This will vary according to what full screen mode is selected!
		 * @return The size of the window.
		 */
		glm::uvec2 GetSize() const;
		/**
		 * @brief Check if this is the end of the software.
		 * @return True if this is the end false if not.
		 */
		bool End() const override;

	public:
		/**
		 * @brief Get the full screen mode.
		 * @return The full screen mode.
		 */
		FullScreenEnum GetFullScreen() const { return fullscreen_; }
		/**
		 * @brief Get the stereo mode.
		 * @return The stereo mode.
		 */
		StereoEnum GetStereo() const { return stereo_; }
		/**
		 * @brief Get the interocular distance in meter.
		 * @return The interocular distance in meter.
		 */
		float GetInterocularDistance() const { return interocular_distance_; }
		/**
		 * @brief Get focus point.
		 * @return The the focus point.
		 */
		glm::vec3 GetFocusPoint() const { return focus_point_; }
		/**
		 * @brief Is right and left inverted?
		 * @return True if they are inverted.
		 */
		bool IsInvertLeftRight() const { return invert_left_right_; }

	private:
		glm::uvec2 size_;
		glm::uvec2 border_less_size_;
		FullScreenEnum fullscreen_ = FullScreenEnum::WINDOW;
		StereoEnum stereo_ = StereoEnum::NONE;
		struct ResolutionElement {
			glm::uvec2 values;
			std::string name;
		};
		std::vector<std::string> resolution_items_;
		std::vector<std::string> fullscreen_items_;
		std::vector<std::string> stereo_items_;
		const std::vector<ResolutionElement> resolutions_ = {
			{{640, 480}, "VGA"},
			{{800, 600}, "SVGA"},
			{{1024, 768}, "XGA"},
			{{1280, 720}, "WXGA-H"},
			{{1920, 1080}, "FHD"},
			{{2560, 1080}, "UW-FHD"},
			{{2560, 1440}, "QHD"},
			{{3440, 1440}, "UW-QHD"},
			{{3840, 2160}, "4K UHD-1"},
			{{7680, 4320}, "8K UHD-2"},
		};
		// Has to correspond to the order in the constructor.
		const std::vector<FullScreenEnum> fullscreen_mode_ = {
			FullScreenEnum::WINDOW,
			FullScreenEnum::FULLSCREEN,
			FullScreenEnum::FULLSCREEN_DESKTOP,
		};
		// Has to correspond to the order in the constructor.
		const std::vector<StereoEnum> stereo_mode_ = {
			StereoEnum::NONE,
			StereoEnum::HORIZONTAL_SPLIT,
			StereoEnum::HORIZONTAL_SIDE_BY_SIDE,
		};
		int resolution_selected_ = 0;
		int fullscreen_selected_ = 0;
		int stereo_selected_ = 0;
		float hppi_ = -1.0f;
		float vppi_ = -1.0f;
		bool invert_left_right_ = false;
		float interocular_distance_ = .065f;
		glm::vec3 focus_point_ = glm::vec3(0, 0, 1);
		std::string name_;
		bool end_ = true;
	};

}  // End namespace frame::gui.
