/**
 * @file QueryAccessSet.ixx
 * @brief Traits for providing information about Read/Write sets of underlying systems.
 */
module;

#include <tuple>

export module helios.ecs.entity.QueryAccessSet;

import helios.core.common.types;
import helios.core.common.traits;


export namespace helios::ecs::entity {
    /**
     * @brief Template for providing HandleList_type containing unique handles used for read access of component data.
     * @tparam TReadComponents
     */
    template <typename... TReadComponents>
    struct ReadSet {
        static constexpr std::uint32_t size = sizeof...(TReadComponents);
        using list = core::common::types::TypeList<TReadComponents...>;
    };

    template<typename ... TReadComponents>
    struct ReadSet<core::common::types::TypeList<TReadComponents...>> : ReadSet<TReadComponents...>{};

    /**
     * @brief Template for providing HandleList_type containing unique handles used for write access of component data.
     * @tparam TWriteComponents
     */
    template <typename... TWriteComponents>
    struct WriteSet {
        static constexpr std::uint32_t size = sizeof...(TWriteComponents);
        using list = core::common::types::TypeList<TWriteComponents...>;
    };

    template <typename... TWriteComponents>
    struct WriteSet<core::common::types::TypeList<TWriteComponents...>> : WriteSet<TWriteComponents...>{};

    template <typename THandle, typename TRead, typename TWrite>
    struct QueryAccessSet;

    /**
     * @brief QueryAccessSet to definining typemembers that provide static information about read/write access of a system.
     *
     * @tparam TReadComponents Components to read from.
     * @tparam TWriteComponents Components to write to.
     */
    template <typename THandle, typename... TReadComponents, typename... TWriteComponents>
    struct QueryAccessSet<THandle, ReadSet<TReadComponents...>, WriteSet<TWriteComponents...>> {

        static_assert(
            !std::same_as<THandle, void>,
            "QueryAccessSet has void HandleType"
        );
        using ReadSet = ReadSet<TReadComponents...>;
        using WriteSet = WriteSet<TWriteComponents...>;

        using HandleType = THandle;
    };

} // namespace helios::ecs::entity::traits