module;

#include <cassert>
#include <exception>

export module helios.ecs.common.types:EntityHandleValue;

import :HandleTypeId;
import :TypeDefs;

export namespace helios::ecs::common::types {

    class EntityHandleValue {

        HandleTypeId handleTypeId_;

        EntityId entityId_;
        VersionId versionId_;

    public:

        template<typename THandle>
        explicit EntityHandleValue(THandle handle)
        : handleTypeId_(HandleTypeId::template id<THandle>()),
        entityId_(handle.entityId()), versionId_(handle.versionId()) {}

        template<typename THandle>
        THandle get() const {
            if (handleTypeId_ != HandleTypeId::template id<THandle>()) {
                assert(false && "Handle type mismatch");
                std::terminate();
            }

            return THandle{entityId_, versionId_};
        }

    };

}