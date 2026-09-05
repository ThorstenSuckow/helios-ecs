/**
 * @file EntityAccessSet.ixx
 * @brief Traits for providing information about Read/Write sets of underlying systems.
 */
module;

#include <tuple>

export module helios.ecs.entity.EntityAccessSet;

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
    using ComponentList = core::common::types::TypeList<TReadComponents...>;
    using HandleList = core::common::traits::UniqueList<
        core::common::types::TypeList<>,
        core::common::types::TypeList<typename TReadComponents::HandleType...>>::list;
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
    using ComponentList = core::common::types::TypeList<TWriteComponents...>;
    using HandleList = core::common::traits::UniqueList<core::common::types::TypeList<>, core::common::types::TypeList<typename TWriteComponents::HandleType...>>::list;
};

template <typename... TWriteComponents>
struct WriteSet<core::common::types::TypeList<TWriteComponents...>> : WriteSet<TWriteComponents...>{};

template <typename TRead, typename TWrite>
struct EntityAccessSet;

/**
 * @brief EntityAccessSet to definining typemembers that provide static information about read/write access of a system.
 *
 * @tparam TReadComponents Components to read from.
 * @tparam TWriteComponents Components to write to.
 */
template <typename... TReadComponents, typename... TWriteComponents>
struct EntityAccessSet<ReadSet<TReadComponents...>, WriteSet<TWriteComponents...>> {

    using ReadComponentSet = ReadSet<TReadComponents...>;
    using WriteComponentSet = WriteSet<TWriteComponents...>;

    using ReadHandleList = ReadComponentSet::HandleList;
    using WriteHandleList = WriteComponentSet::HandleList;

    using AccessHandleList =
        core::common::traits::UniqueList<
            ReadHandleList,
            typename core::common::traits::UniqueList<core::common::types::TypeList<>, WriteHandleList>::list>::list;

    using ReadHandles = core::common::traits::ListToTuple<ReadHandleList>::tuple;

    using WriteHandles = core::common::traits::ListToTuple<WriteHandleList>::tuple;

    using AccessHandles = core::common::traits::ListToTuple<AccessHandleList>::tuple;
};

} // namespace helios::ecs::common::types