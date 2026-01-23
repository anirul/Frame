#pragma once

#include "frame/json/proto.h"
#include "frame/name_interface.h"
#include <concepts>
#include <google/protobuf/message.h>
#include <type_traits>

namespace frame
{

/**
 * @class SerializeInterface
 * @brief This is the interface for all object that can be serialized.
 * @tparam ProtoT: The proto type to serialize (check with concepts).
 */
template <typename ProtoT>
    requires json::ProtoMessage<ProtoT>
struct SerializeInterface : public NameInterface
{
    virtual ~SerializeInterface() noexcept = default;
    /**
     * @brief Is serialize enable?
     * @return Serialize enabled?
     */
    virtual bool SerializeEnable() const = 0;
    /**
     * @brief Set is serialize enabled?
     * @param is_serialize: Is serailize enabled?
     */
    virtual void SetSerializeEnable(bool is_serialize) = 0;
    /**
     * @brief Serialize the object.
     * @return The serialized proto object.
     */
    virtual ProtoT ToProto() const = 0;
    /**
     * @brief Deserialize the object.
     * @param proto: The serialized object.
     */
    virtual void FromProto(ProtoT&& proto) = 0;
    /**
     * @brief Get the data.
     * @return The data.
     */
    virtual ProtoT& GetData() = 0;
	//! @brief Same but const.
    virtual const ProtoT& GetData() const = 0;
};

} // End namespace frame.
