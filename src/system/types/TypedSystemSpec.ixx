/**
 * @file TypedSystemSpec.ixx
 * @brief Compile-time descriptor pairing a system type with its construction arguments.
 */
module;

#include <tuple>

export module helios.ecs.system.types:TypedSystemSpec;

export namespace helios::ecs::system::types {

    /**
     * @brief Stores a system type together with the arguments needed to construct it.
     *
     * @tparam TSystem The concrete system type.
     * @tparam Args    Constructor argument types captured at the call site.
     *
     * @see TypedSystem
     */
    template<typename TSystem, typename ...Args>
    struct TypedSystemSpec {

        using System_type = TSystem;

        std::tuple<std::decay_t<Args>...> args;
    };

    /**
     * @brief Convenience factory that deduces template arguments for `TypedSystemSpec`.
     *
     * @tparam TSystem The system type to wrap.
     * @tparam Args    Deduced argument types.
     * @param  args    Arguments forwarded into the spec's tuple.
     * @return A `TypedSystemSpec<TSystem, Args...>` holding the provided arguments.
     */
    template<typename TSystem, typename ...Args>
    auto TypedSystem(Args&&...args) {
        return TypedSystemSpec<TSystem, Args...>{
            std::forward_as_tuple(args...)
        };

    }

}