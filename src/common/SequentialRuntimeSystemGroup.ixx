module;

#include <tuple>

export module helios.ecs.common.SequentialRuntimeSystemGroup;

import helios.core.common;

export namespace helios::ecs::common {

    template<typename ... TRuntimeSystems>
    class Sequential {

        private:
        std::tuple<TRuntimeSystems...> runtimeSystems_;

        public:

        using list = core::common::types::TypeList<
            TRuntimeSystems...
        >;

        template<typename T>
        auto& runtimeSystemFor() {
            return std::get<T>(runtimeSystems_);
        }

        explicit Sequential() = default;

        explicit Sequential(TRuntimeSystems ...runtimeSystems)
            : runtimeSystems_(std::move(runtimeSystems)...) {}

    } ;



};