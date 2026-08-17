/**
 * @file ContextRef.ixx
 * @brief Tagged runtime context reference with safe type-checked access.
 */
module;

#include <cassert>

export module helios.ecs.common.types:ContextRef;

import helios.core.common.types;
import :ContextTypeId;


export namespace helios::ecs::common::types {


    /**
     * @brief Non-owning, type-erased reference to a runtime context.
     *
     * Provides checked access to the concrete type across type-erased boundaries..
     *
     * @tparam TPhase
     *
     * @see manager::Manager
     */
    class ContextRef {

        ContextTypeId id_{helios::core::common::types::no_init};

        void* ptr_{};

    public:

        ContextRef() = default;

        template<typename TConcreteContext>
        explicit ContextRef(TConcreteContext& ctx) :
            id_(ContextTypeId::template id<TConcreteContext>()),
            ptr_(&ctx)
        {}

        [[nodiscard]] bool isValid() const noexcept {
            return ptr_ != nullptr;
        }

        template<typename TRequiredContext>
        [[nodiscard]] TRequiredContext* tryGet() const noexcept {

            assert(isValid() && "ContextRef is not valid. Ensure it was constructed with a valid context.");

            auto requiredId = ContextTypeId::id<TRequiredContext>();

            if (id_ != requiredId) [[unlikely]] {
                return nullptr;
            }

            return static_cast<TRequiredContext*>(ptr_);
        }


    };


}
