/**
 * @file NullContextTypes.ixx
 * @brief NullContextTypes for initialization and flushing.
 */
module;

export module helios.ecs.common.types:NullContextTypes;

export namespace helios::ecs::common::types {

    struct NullInitContext {
        NullInitContext() = default;
    };
    struct NullFlushContext {
        NullFlushContext() = default;
    };

}