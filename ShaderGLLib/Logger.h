#pragma once

#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace sgl {

	class Logger 
	{
	private:
		Logger();

	public:
		virtual ~Logger();

	public:
		static Logger& GetInstance();
		std::shared_ptr<spdlog::logger> operator->();

	private:
		std::shared_ptr<spdlog::logger> logger_ptr_ = nullptr;
	};

} // End namespace sgl.
