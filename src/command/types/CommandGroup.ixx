module;


export module helios.ecs.command.types:CommandGroup;


export namespace helios::ecs::command::types {
    /**
     * @brief Tag component for identifying command groups.
     *
     * Helps with identifying AddComponentCommand<Cmp<Handle>> belonging to
     * CommandGroup<AddComponent, Handle>.
     *
     * @tparam TCommand
     * @tparam THandle
     */
    template<
        template<typename> typename TCommand,
        typename THandle
    >
    struct CommandGroup {};
}