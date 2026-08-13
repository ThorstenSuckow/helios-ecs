module;

export module helios.ecs.types:ContextPair;


export namespace helios::ecs::types {

    template<typename T>
    concept ContextPairLike = requires
    {
        typename T::LeftContextType;
        typename T::RightContextType;
    };

    /**
     * @brief Helper struct for grouping distinct pairs of Contexts.
     *
     * @tparam TContextLft
     * @tparam TContextRgt
     */
    template<typename TContextLft, typename TContextRgt>
    struct ContextPair {
        using LeftContextType = TContextLft;
        using RightContextType = TContextRgt;
    };

    template<typename ... TContextPairs>
    requires (ContextPairLike<TContextPairs> && ...)
    struct ContextPairList{};
}