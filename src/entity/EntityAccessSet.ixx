module;

#include <tuple>

export module helios.ecs.entity.EntityAccessSet;

import helios.core.common.types;



namespace {

using namespace helios::core::common::types;
/**
 * @brief Primary template to list/tuple conversion.
 * @tparam TList
 */
template <typename TList>
struct ToTuple {};

/**
 * @brief Partial specialization of ToTuple for HandleList.
 * @tparam TElements The element types contained in HandleList.
 */
template <typename... TElements>
struct ToTuple<TypeList<TElements...>> {
    using type = std::tuple<TElements...>;
};

/**
 * @brief Primary template for Contains; membership test is available with its partial specialization.
 * @tparam THandle The handle to check for.
 * @tparam TTail The list of handles to check within.
 */
template <typename THandle, typename TTail>
struct Contains {};

/**
 * @brief Partial specialization of Contains for checking if THandle appears in HandleList<TTail...>.
 * @tparam THandle The handle for comparison.
 * @tparam TTail The element types contained in HandleList.
 */
template <typename THandle, typename... TTail>
struct Contains<THandle, TypeList<TTail...>> {
    static constexpr bool value = (std::is_same_v<THandle, TTail> || ...);
};

/**
 * @brief Primary template for AppendIfUnique.
 * @tparam THandle The handle for comparison.
 * @tparam TList The list to check for duplicates.
 * @tparam exists true if THandle already exists in TList, false otherwise.
 */
template <typename THandle, typename TList, bool exists = Contains<THandle, TList>::value>
struct AppendIfUnique {};

/**
 * @brief Partial specialization of AppendIfUnique for when THandle already exists in TTail.
 * Defines `list` as HandleList w/o repeated THandle.
 * @tparam THandle The handle for comparison.
 * @tparam TTail The element types contained in HandleList.
 */
template <typename THandle, typename... TTail>
struct AppendIfUnique<THandle, TypeList<TTail...>, true> {
    using list = TypeList<TTail...>;
};

/**
 * @brief Partial specialization for AppendIfUnique for when THandle does not appear in TTail.
 * Defines list as a list that contains THandle and the remaining elememts.
 * @tparam THandle The handle for comparison.
 * @tparam TTail The element types contained in HandleList.
 */
template <typename THandle, typename... TTail>
struct AppendIfUnique<THandle, TypeList<TTail...>, false> {
    using list = TypeList<TTail..., THandle>;
};

/**
 * @brief Primary template for deducing a list of unique handles.
 * @tparam TUniqueList The resulting unique list (accumulator).
 * @tparam TInputList The input list that may contains multiple equal handle types.
 */
template <typename TUniqueList, typename TInputList>
struct UniqueHandleList {};

/**
 * @brief Partial specialization for empty HandleList<>, in which case the elements for TUniqueList are used for list.
 * @tparam TUniqueList The unique list of handles
 */
template <typename... TUniqueList>
struct UniqueHandleList<TypeList<TUniqueList...>, TypeList<>> {
    using list = TypeList<TUniqueList...>;
};

/**
 * @brief Partial specialization for UniqueHandleList that recursively build list membertypedef
 * out of HandleList with at least one entry.
 *
 * @tparam TUniqueList The resulting unique handle list.
 * @tparam THead First entry of input list.
 * @tparam TTail The remaining elements of the HandleList.
 */
template <typename... TUniqueList, typename THead, typename... TTail>
struct UniqueHandleList<TypeList<TUniqueList...>, TypeList<THead, TTail...>> {
    using list = UniqueHandleList<
        typename AppendIfUnique<THead, TypeList<TUniqueList...>>::list,
        TypeList<TTail...>
    >::list;
};
} // namespace

export namespace helios::ecs::entity {
/**
 * @brief Template for providing HandleList_type containing unique handles used for read access of component data.
 * @tparam TReadComponents
 */
template <typename... TReadComponents>
struct Read {
    using ComponentList = TypeList<TReadComponents...>;
    using HandleList = UniqueHandleList<TypeList<>, TypeList<typename TReadComponents::Handle_type...>>::list;
};

/**
 * @brief Template for providing HandleList_type containing unique handles used for write access of component data.
 * @tparam TWriteComponents
 */
template <typename... TWriteComponents>
struct Write {
    using ComponentList = TypeList<TWriteComponents...>;
    using HandleList = UniqueHandleList<TypeList<>, TypeList<typename TWriteComponents::Handle_type...>>::list;
};

template <typename TRead, typename TWrite>
struct EntityAccessSet;

/**
 * @brief EntityAccessSet to definining typemembers that provide static information about read/write access of a system.
 *
 * @tparam TReadComponents Components to read from.
 * @tparam TWriteComponents Components to write to.
 */
template <typename... TReadComponents, typename... TWriteComponents>
struct EntityAccessSet<Read<TReadComponents...>, Write<TWriteComponents...>> {

    using ReadComponentSet = Read<TReadComponents...>;
    using WriteComponentSet = Write<TWriteComponents...>;

    using ReadHandleList = ReadComponentSet::HandleList;
    using WriteHandleList = WriteComponentSet::HandleList;

    using AccessHandleList =
        UniqueHandleList<
            ReadHandleList,
            typename UniqueHandleList<TypeList<>, WriteHandleList>::list>::list;

    using ReadHandles = ToTuple<ReadHandleList>::type;

    using WriteHandles = ToTuple<WriteHandleList>::type;

    using AccessHandles = ToTuple<AccessHandleList>::type;
};

} // namespace helios::ecs::common::types