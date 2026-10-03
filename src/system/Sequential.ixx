module;

#include <tuple>

export module helios.ecs.system.Sequential;

import helios.core.common;
import helios.ecs.common;

export namespace helios::ecs::system {

    template<typename ... TSystems>
    class Sequential {

        private:
        std::tuple<TSystems...> systems_;

        public:

        using list = core::common::types::TypeList<
            TSystems...
        >;

        template<typename T>
        auto& systemFor() {
            return std::get<T>(systems_);
        }

        explicit Sequential() = default;

        explicit Sequential(TSystems ...systems)
            : systems_(std::move(systems)...) {}

    } ;



};