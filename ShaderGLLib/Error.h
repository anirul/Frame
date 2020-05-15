#pragma once

#include <memory>
#include <string>

namespace sgl {

	class Error
	{
	public:
		Error(const Error&) = delete;
		Error& operator=(const Error&) = delete;
		Error(Error&&) = delete;
		Error& operator=(Error&&) = delete;

	public:
		void Display(
			const std::string& file = "", 
			const int line = -1) const;
		void CreateError(
			const std::string& error,
			const std::string& file,
			const int line = -1) const;

	public:
		bool AlreadyRaized() const { return already_raized_; }
		static Error& GetInstance()
		{
			static Error error_;
			return error_;
		}
		static void SetWindowPtr(void* window_ptr) { window_ptr_ = window_ptr; }

	protected:
		std::string GetLastError() const;

	private:
		Error() = default;
		static void* window_ptr_;
		static bool already_raized_;
	};

} // End namespace sgl.
