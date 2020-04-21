#pragma once

#include <memory>
#include <string>

namespace sgl {

	class Error
	{
	public:
		void Display(
			const std::string& file = "", 
			const int line = -1) const;
		void DisplayShader(
			unsigned int shader,
			const std::string& file = "",
			const int line = -1) const;
		static std::shared_ptr<Error> GetInstance();

	public:
		static void SetWindowPtr(void* window_ptr) { window_ptr_ = window_ptr; }

	protected:
		std::string GetLastError() const;

	private:
		static std::shared_ptr<Error> instance_;
		static void* window_ptr_;
	};

} // End namespace sgl.
