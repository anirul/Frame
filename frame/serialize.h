#pragma once

#include "frame/serialize_interface.h"

namespace frame
{

template <typename ProtoT>
    requires json::ProtoMessage<ProtoT>
class Serialize : public SerializeInterface<ProtoT>
{
  public:
    bool SerializeEnable() const final
    {
        return enable_serialize_;
    }
    void SetSerializeEnable(bool is_serialize) final
    {
        enable_serialize_ = is_serialize;
    }
    ProtoT ToProto() const final
    {
        return data_;
    }
    void FromProto(ProtoT&& proto) final
    {
        data_ = std::move(proto);
    }
    std::string GetName() const final
    {
        return data_.name();
	}
    void SetName(const std::string& name) final
    {
		data_.set_name(name);
    }
    const ProtoT& GetData() const final
    {
        return data_;
    }
    ProtoT& GetData() final
    {
        return data_;
    }

  protected:
    ProtoT data_;
    bool enable_serialize_ = false;
};

} // End namespace frame.
