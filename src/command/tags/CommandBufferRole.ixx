/**
 * @file CommandBufferRole.ixx
 * @brief Tag type for identifying CommandBuffer-role classes at compile time.
 */
module;

export module helios.ecs.command.tags:CommandBufferRole;

export namespace helios::ecs::command::tags {

    /**
     * @brief Compile-time tag identifying a class as a CommandBuffer
     */
    struct CommandBufferRole{};
}