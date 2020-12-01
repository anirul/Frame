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
		const std::shared_ptr<spdlog::logger> operator->() const;

	private:
		std::shared_ptr<spdlog::logger> logger_ptr_ = nullptr;
	};

} // End namespace sgl.
