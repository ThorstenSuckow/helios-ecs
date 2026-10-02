module;


export module helios.ecs.system.Sequential;

import helios.core.common;
import helios.ecs.common;

export namespace helios::ecs::system {

    template<typename ... TSystems>
    struct Sequential {

        using list = core::common::types::TypeList<
            TSystems...
        >;
    } ;



};