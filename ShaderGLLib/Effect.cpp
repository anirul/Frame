#include "Effect.h"

namespace sgl {

	Effect::Effect(const frame::proto::Effect& effect_proto)
	{

	}

	void Effect::Startup(std::pair<std::uint32_t, std::uint32_t> size)
	{

	}

	void Effect::Draw(const double dt /*= 0.0*/)
	{

	}

	void Effect::Delete()
	{

	}

	const std::string Effect::GetName() const
	{
		return name_;
	}

}