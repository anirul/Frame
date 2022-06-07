#pragma once

#include <memory>

#include "Frame/EntityId.h"
#include "Frame/NameInterface.h"

namespace frame {

/**
 * @class StaticMeshInterface
 * @brief A static mesh is a mesh that cannot change over time (no skeleton).
 */
struct StaticMeshInterface : public NameInterface {
    //! @brief Virtual destructor.
    virtual ~StaticMeshInterface() = default;
    /**
     * @brief Set the material id.
     * @param id: the material id to be set.
     */
    virtual void SetMaterialId(EntityId id) = 0;
    /**
     * @brief Get material id.
     * @return Current material id.
     */
    virtual EntityId GetMaterialId() const = 0;
    /**
     * @brief Get point buffer id.
     * @return Current point buffer id.
     */
    virtual EntityId GetPointBufferId() const = 0;
    /**
     * @brief Get normal buffer id.
     * @return Current normal buffer id.
     */
    virtual EntityId GetNormalBufferId() const = 0;
    /**
     * @brief Get texture buffer id.
     * @return Current texture buffer id.
     */
    virtual EntityId GetTextureBufferId() const = 0;
    /**
     * @brief Get index buffer id (triangle based).
     * @return Current index buffer id.
     */
    virtual EntityId GetIndexBufferId() const = 0;
    /**
     * @brief This is the size in bytes! so if you need the element size just divide this number by
     * the sizeof(std::int32_t).
     * @return Size of the index buffer in bytes!
     */
    virtual std::size_t GetIndexSize() const = 0;
    /**
     * @brief Check if depth buffer is cleared or not.
     * @return Is depth buffer cleared?
     */
    virtual bool IsClearBuffer() const       = 0;
};

}  // End namespace frame.
