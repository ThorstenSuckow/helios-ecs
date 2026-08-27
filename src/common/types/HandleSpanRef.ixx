module;

#include <span>
#include <cassert>
#include <cstddef>
#include <exception>

export module helios.ecs.common.types:HandleSpanRef;

import :HandleTypeId;

export namespace helios::ecs::common::types {

    class HandleSpanRef {

        HandleTypeId handleTypeId_;

        const void* data_;
        std::size_t size_;

    public:

        template<typename THandle>
        static constexpr HandleSpanRef makeEmpty() {
            return HandleSpanRef(std::span<const THandle>{});
        }

        template<typename THandle>
        explicit HandleSpanRef(const std::vector<THandle>& handles)
        : HandleSpanRef{std::span<const THandle>{handles}} {}


        template<typename THandle>
        explicit HandleSpanRef(std::span<const THandle> handles)
        : handleTypeId_(HandleTypeId::template id<THandle>()),
        data_(handles.data()), size_(handles.size()) {}

        template<typename THandle>
        std::span<const THandle> get() const {
            if (handleTypeId_ != HandleTypeId::template id<THandle>()) {
                assert(false && "Handle type mismatch");
                std::terminate();
            }

            return {
                static_cast<const THandle*>(data_), size_
            };
        }

    };

}