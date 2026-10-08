module;

#include <type_traits>

export module helios.ecs.scheduling.traits:SystemReturnType;

import helios.ecs.common.InvocationContext;
import helios.ecs.system.concepts;

export namespace helios::ecs::scheduling::traits {

    template<typename TSystem>
    struct SystemReturnType {

        using SystemType = std::remove_cvref_t<TSystem>;

        static consteval auto updateFunction() {
            if constexpr (ecs::system::concepts::IsCallableSystem<SystemType>) {
                return &SystemType::operator();
            } else {
                return &SystemType::update;
            }
        }

        using InvocationContext = common::InvocationContext<decltype(updateFunction())>;

        using type = typename InvocationContext::ReturnType;
    };

};