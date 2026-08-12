module;

export module helios.ecs.types:ContextPair;


export namespace helios::ecs::types {
    /**
     * @brief Helper struct for grouping distinct pairs of Contexts.
     *
     * @tparam TContextLft
     * @tparam TContextRgt
     */
    template<typename TContextLft, typename TContextRgt>
    struct ContextPair {
        using LeftContext_type = TContextLft;
        using RightContext_type = TContextRgt;
    };;
}