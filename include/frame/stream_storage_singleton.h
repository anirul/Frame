#pragma once

#include <map>
#include <memory>
#include <string>

#include "frame/json/proto.h"
#include "frame/logger.h"
#include "frame/stream_interface.h"

namespace frame {

/**
 * @class StreamStorageSingleton
 * @brief This class is a singleton that store all the stream interface that are used in the app.
 */
class StreamStorageSingleton {
   public:
    static StreamStorageSingleton& GetInstance();
    /**
     * @brief Set stream buffer callback.
     * @param callback: The function that is suppose to return the buffer interface and the time at
     * which it was captured.
     */
    void SetStreamBufferCallback(const std::string& connect,
                                 std::unique_ptr<StreamInterface<float>>&& callbacks);
    /**
     * @brief Set stream texture callback.
     * @param callback: The function that is suppose to return the texture interface and the time at
     * which it was captured.
     */
    void SetStreamTextureCallback(const std::string& connect,
                                  std::unique_ptr<StreamInterface<std::uint8_t>>&& callbacks);
    /**
     * @brief Set stream uniform float callback.
     * @param callback: The callback that is suppose to return the value of the uniform float.
     */
    void SetStreamUniformFloatCallback(const std::string& connect,
                                       std::unique_ptr<StreamInterface<float>>&& callbacks);
    /**
     * @brief Set the stream uniform int callback.
     * @param callback: The callback that is suppose to return the value of the uniform int.
     */
    void SetStreamUniformIntCallback(const std::string& connect,
                                     std::unique_ptr<StreamInterface<std::int32_t>>&& callbacks);
    /**
     * @brief Set the root name id.
     * @param root_id: The root name id.
     */
    void SetRootId(const std::string& root_id);
    /**
     * @brief Get root name id.
     * @return The root name id.
     */
    std::string GetRootId() const;
    /**
     * @brief Add an id to the list of ids.
     * @param id: The id to be added to the list.
     * @return Number corresponding to the new id.
     */
    std::uint32_t AddId(const std::string& id);
    /**
     * @brief Get the number corresponding to an id.
     * @param id: Get the id.
     * @return Number corresponding to the id.
     */
    std::uint32_t GetIdNumber(const std::string& id) const;
    /**
     * @brief Connect to a texture interface stream provider.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A stream texture interface pointer.
     */
    StreamInterface<float>* ConnectToNoneStreamBufferInterface(const std::string& connect);
    /**
     * @brief Connect to a matrix interface stream provider.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A stream matrix interface pointer.
     */
    std::vector<StreamInterface<std::uint8_t>*> ConnectToAllStreamTextureInterface(
        const std::string& connect);
    /**
     * @brief Connect to a buffer interface stream provider.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A stream buffer interface pointer.
     */
    StreamInterface<float>* ConnectToNoneStreamUniformFloatInterface(const std::string& connect);
    /**
     * @brief Connect to all stream uniform float interface.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A vector of stream interface pointers.
     */
    std::vector<StreamInterface<float>*> ConnectToAllStreamUniformFloatInterface(
        const std::string& connect);
    /**
     * @brief Connect to a buffer interface stream provider.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A stream buffer interface pointer.
     */
    StreamInterface<std::int32_t>* ConnectToNoneStreamUniformIntInterface(
        const std::string& connect);
    /**
     * @brief Connect to all stream uniform float interface.
     * @param connect: The representation of the hardware item you are looking for.
     * @return A vector of stream interface pointers.
     */
    std::vector<StreamInterface<std::int32_t>*> ConnectToAllStreamUniformIntInterface(
        const std::string& connect);
    /**
     * @brief Get the corresponding item name from stream name.
     * @param name: The stream name.
     * @return The corresponding item name.
     */
    std::string GetItemNameFromStreamName(const std::string& name) const;
    /**
     * @brief Has the stream name in the storage.
	 * @param name: Name to be checked.
     * @return True if the name is in the storage.
     */
    bool HasStreamName(const std::string& name) const;

   protected:
    template <typename T>
    StreamInterface<T>* ConnectToSpecificStreamInterface(
        const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
        const std::string& connect, std::uint32_t stream_id);
    template <typename T>
    StreamInterface<T>* ConnectToNoneStreamInterface(
        const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
        const std::string& connect);
    template <typename T>
    std::vector<StreamInterface<T>*> ConnectToAllStreamInterface(
        const std::map<std::string, std::unique_ptr<StreamInterface<T>>>& map,
        const std::string& connect);

   private:
    StreamStorageSingleton() = default;
    std::map<std::string, std::unique_ptr<StreamInterface<float>>> buffer_callback_map_;
    std::map<std::string, std::unique_ptr<StreamInterface<float>>> uniform_float_callback_map_;
    std::map<std::string, std::unique_ptr<StreamInterface<std::int32_t>>> uniform_int_callback_map_;
    std::map<std::string, std::unique_ptr<StreamInterface<std::uint8_t>>> texture_callback_map_;
    std::map<std::string, std::uint32_t> id_number_map_;
    std::map<std::string, std::string> name_stream_map_;
    std::string default_id_;
    frame::Logger& logger_ = Logger::GetInstance();
};

}  // End namespace frame.
