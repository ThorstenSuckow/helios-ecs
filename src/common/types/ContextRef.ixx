/**
 * @file ContextRef.ixx
 * @brief Tagged runtime context reference with safe type-checked access.
 */
module;

#include <concepts>

export module helios.ecs.common.types:ContextRef;

import :ContextTypeId;


export namespace helios::ecs::common::types {

    struct Execution{};
    struct Init{};
    struct Flush{};
    struct Update{};

    template<typename TPhase>
    concept IsRuntimeContext =
        std::same_as<TPhase, Execution>
    || std::same_as<TPhase, Init>
    || std::same_as<TPhase, Flush>
    || std::same_as<TPhase, Update>;


    /**
     * @brief Non-owning, type-erased reference to a runtime context.
     *
     * Provides checked access to the concrete type across type-erased boundaries..
     *
     * @tparam TPhase
     *
     * @see manager::Manager
     */
    template<typename TPhase>
    requires IsRuntimeContext<TPhase>
    class ContextRef {

        ContextTypeId<TPhase> id_;

        void* ptr_{};

    public:

        template<typename TConcreteContext>
        explicit ContextRef(TConcreteContext& ctx) :
            id_(ContextTypeId<TPhase>::template id<TConcreteContext>()),
            ptr_(&ctx)
        {}


        template<typename TRequiredContext>
        [[nodiscard]] TRequiredContext* tryGet() const noexcept {

            auto requiredId = ContextTypeId<TPhase>::template id<TRequiredContext>();

            if (id_ != requiredId) [[unlikely]] {
                return nullptr;
            }

            return static_cast<TRequiredContext*>(ptr_);
        }


    };


}
