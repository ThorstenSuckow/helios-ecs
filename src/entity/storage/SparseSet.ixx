/**
 * @file SparseSet.ixx
 * @brief Generic sparse set data structure for efficient entity-keyed storage.
 */
module;

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <span>
#include <vector>

export module helios.ecs.entity.storage.SparseSet;

import helios.ecs.common.types;

using namespace helios::ecs::common::types;
export namespace helios::ecs::entity::storage {

/**
 * @brief Type-erased sparse-set interface.
 *
 * Enables polymorphic access to sparse sets of different component types.
 */
class SparseSetBase {

public:
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~SparseSetBase() = default;

    /**
     * @brief Removes the element at the given index.
     *
     * @param id The EntityId of the element to remove.
     *
     * @return True if the element was removed, false if not found or
     *         removal was cancelled.
     */
    virtual bool remove(EntityId id) = 0;

    /**
     * @brief Clears all elements from the sparse set.
     *
     * This operation removes all elements and resets the internal
     * data structures, effectively making the set empty.
     */
    virtual void clear() = 0;

    /**
     * @brief Checks whether an element exists for the specified EntityId.
     *
     * @param id The EntityId to test.
     *
     * @return True if the set contains the EntityId.
     */
    [[nodiscard]] virtual bool contains(EntityId id) const = 0;

    /**
     * @brief Returns a raw void pointer to the element at the given index.
     *
     * @param id The EntityId to look up.
     *
     * @return Raw pointer to the element, or `nullptr` if not found.
     */
    [[nodiscard]] virtual void* raw(EntityId id) = 0;

    /**
     * @brief Copy-constructs a component from one entity slot into another.
     *
     * @details Copies a component for the specified sourceId to the targetId. Checks
     * if the component is copy-constructible. Resulting data is depending on the
     * source components constructor.
     *
     * @param sourceId The EntityId of the source component.
     * @param targetId The EntityId of the target component.
     *
     * @return Pointer to the copy component, or `nullptr` if cloning failed, e.g.
     * because the component is not copy-constructible or the target already contains the component.
     */
    [[nodiscard]] virtual void* copy(EntityId sourceId, EntityId targetId) = 0;

    /**
     * @brief Copies the data ot the sourceId to the targetSparseSet at targetId.
     *
     * @param sourceId The source entity id of the data to copy.
     * @param targetSparseSet The target SparseSet.
     * @param targetId The targetId to use as the slot index for the new copied data.
     * @return
     */
    [[nodiscard]] virtual bool copyTo(EntityId sourceId, SparseSetBase& targetSparseSet, EntityId targetId) const = 0;

    /**
     * @brief Resets the specified entity to the data from sourceId found in the sourceSparseSet.
     *
     * @details Components must satisfy the std::is_copy_assignable-constraint.
     *
     * @param targetId The target entity id to reset.
     * @param sourceSparseSet The source SparseSet.
     * @param sourceId The source entity id to copy from.
     *
     * @return True if the reset was successful, false otherwise.
     */
    [[nodiscard]] virtual bool resetTo(EntityId targetId, SparseSetBase& sourceSparseSet, EntityId sourceId) = 0;

    /**
     * @brief Creates a new unique ptr to a SparseSetBase.
     *
     * @details Creates a unique ptr to a SparseSetBase. Used in implementation to
     * create a SparseSet for the concrete data type, while callable via the base class API.
     *
     * @return A unique ptr to the SparseSet.
     */
    [[nodiscard]] virtual std::unique_ptr<SparseSetBase> makeEmpty() const = 0;

    /**
     * @brief Returns the highest `EntityId` currently stored in this set.
     *
     * Returns `Tombstone` if the set is empty.
     *
     * @return The maximum `EntityId`, or `Tombstone` if the set is empty.
     */
    [[nodiscard]] virtual EntityId maxEntityId() const noexcept = 0;

    /**
     * @brief Returns the number of elements currently stored in this set.
     *
     * @return Dense element count.
     */
    [[nodiscard]] virtual size_t componentCount() const noexcept = 0;

    /**
     * @brief Returns a non-owning span over all `EntityId`s currently in the set.
     *
     * The span is backed by the internal dense-to-sparse mapping and is
     * invalidated by any mutating operation on the set.
     *
     * @return Span of `EntityId`s in dense storage order.
     */
    [[nodiscard]] virtual std::span<const EntityId> entityIds() const noexcept = 0;

    /**
     * @brief Refreshes internal metadata after mutations.
     *
     * @warning Not thread-safe. Requires exclusive access.
     */
    virtual void finalizeMutations() noexcept = 0;

    /**
     * @brief Reserves capacity for internal storage.
     *
     * @param capacity Requested capacity.
     */
    virtual void reserve(size_t capacity) = 0;
};

/**
 * @brief Sentinel value indicating an empty slot in the sparse array.
 *
 * Aliased from `helios::ecs::common::types::EntityTombstone`.
 */
constexpr auto Tombstone = EntityTombstone;

/**
 * @brief Sparse-set storage for values keyed by `EntityId`.
 *
 * Uses sparse and dense arrays with swap-and-pop removal for O(1) average
 * insert, lookup, and erase.
 *
 * @tparam T Stored value type.
 */
template <typename TComponent>
class SparseSet : public SparseSetBase {
    /**
     * @brief Maps EntityId to dense storage index.
     *
     * Contains `Tombstone` for empty slots.
     */
    std::vector<size_t> sparse_;

    /**
     * @brief Reverse mapping from dense index to EntityId.
     *
     * Used during swap-and-pop removal to update the sparse array.
     */
    std::vector<EntityId> denseToSparse_;

    /**
     * @brief Contiguous storage of elements.
     */
    std::vector<TComponent> storage_;

    /**
     * @brief Max EntityId available in this SparseSet, defaults to `Tombstone`.
     */
    mutable EntityId maxEntityId_ = Tombstone;

    /**
     * @brief Tracks an invalidated previous max id after removals.
     */
    EntityId invalidatedMaxEntityId_ = Tombstone;

    /**
     * @brief Reserved capacity for internal vectors.
     */
    std::size_t capacity_ = 0;

    /**
     * @brief Updates cached max entity id after insertion.
     *
     * @param idx Inserted entity id.
     */
    void updateMaxEntityId(const EntityId idx) {
        if (invalidatedMaxEntityId_ != Tombstone && invalidatedMaxEntityId_ <= idx) {
            invalidatedMaxEntityId_ = Tombstone;
            maxEntityId_ = idx;
            return;
        }

        maxEntityId_ = maxEntityId_ == Tombstone ? idx : std::max(maxEntityId_, idx);
    }

public:
    using Component_type = TComponent;

    /**
     * @brief Default constructor creating an empty sparse set.
     */
    SparseSet() = default;

    /**
     * @brief Constructs a sparse set with pre-allocated capacity.
     *
     * @param capacity The initial capacity to reserve for all internal vectors.
     */
    explicit SparseSet(const size_t capacity) {
        SparseSet::reserve(capacity);
    };

    /**
     * @brief Copy operations are deleted to prevent accidental duplication.
     */
    SparseSet(const SparseSet&) = delete;

    /**
     * @brief Copy assignment is deleted.
     */
    SparseSet& operator=(const SparseSet&) = delete;

    /**
     * @brief Move constructor.
     */
    SparseSet(SparseSet&&) noexcept = default;

    /**
     * @brief Move assignment operator.
     */
    SparseSet& operator=(SparseSet&&) noexcept = default;

    /**
     * @brief Returns the component type id for `TComponent`.
     *
     * @return Type id of this sparse-set component type.
     */
    ComponentTypeId<TComponent> componentTypeId() {
        return ComponentTypeId<typename TComponent::HandleType>::template id<TComponent>();
    };

    /**
     * @brief Set the capacity of the underlying storages.
     *
     * @param capacity The capacity to reserve. In effect only if current capacity is lower than the new capacity.
     */
    void reserve(const size_t capacity) override {
        if (capacity > capacity_) {
            sparse_.reserve(capacity);
            storage_.reserve(capacity);
            denseToSparse_.reserve(capacity);
            capacity_ = capacity;
        }
    }

    /**
     * @brief Constructs and inserts an element at the given index.
     *
     * Forwards arguments to construct `TComponent` in-place.
     *
     * @tparam Args Constructor argument types.
     *
     * @param idx The EntityId to associate with the element.
     * @param args Arguments forwarded to the `TComponent` constructor.
     *
     * @return Pointer to the inserted element, or `nullptr` if the index is already occupied.
     */
    template <typename... Args>
    [[nodiscard]] TComponent* emplace(const EntityId idx, Args&&... args) {

        // already in use
        if (idx < sparse_.size() && sparse_[idx] != Tombstone) {
            return nullptr;
        }

        if (idx >= sparse_.size()) {
            sparse_.resize(idx + 1, Tombstone);
        }

        const auto denseIndex = storage_.size();

        denseToSparse_.push_back(idx);
        storage_.emplace_back(std::forward<Args>(args)...);

        sparse_[idx] = denseIndex;

        updateMaxEntityId(idx);

        return &storage_.back();
    }

    /**
     * @brief Inserts an element at the given index.
     *
     * If the sparse array is too small, it is resized to accommodate the index.
     * Empty slots are filled with `Tombstone`.
     *
     * @param idx The EntityId to associate with the element.
     * @param obj The element to insert (moved).
     *
     * @return Pointer to the inserted element, or `nullptr` if the index is already occupied.
     */
    [[nodiscard]] TComponent* insert(const EntityId idx, TComponent&& obj) {

        // already in use
        if (idx < sparse_.size() && sparse_[idx] != Tombstone) {
            return nullptr;
        }

        if (idx >= sparse_.size()) {
            sparse_.resize(idx + 1, Tombstone);
        }

        const auto denseIndex = storage_.size();

        denseToSparse_.push_back(idx);
        storage_.emplace_back(std::move(obj));

        sparse_[idx] = denseIndex;

        updateMaxEntityId(idx);

        return &storage_.back();
    }

    /**
     * @copydoc SparseSetBase::makeEmpty
     */
    [[nodiscard]] std::unique_ptr<SparseSetBase> makeEmpty() const override {
        return std::make_unique<SparseSet<TComponent>>();
    }

    /**
     * @copydoc SparseSetBase::copy
     */
    [[nodiscard]] void* copy(const EntityId sourceId, const EntityId targetId) override {

        if (copyTo(sourceId, *this, targetId)) {
            return raw(targetId);
        }

        return nullptr;
    }

    /**
     * @copydoc SparseSetBase::makeEmpty
     */
    bool copyTo(const EntityId sourceId, SparseSetBase& targetSparseSet, const EntityId targetId) const override {

        if constexpr (!std::copy_constructible<TComponent>) {
            assert(false && "cannot copy the component, is this intentional?");
            return false;
        } else {
            auto& target = static_cast<SparseSet<TComponent>&>(targetSparseSet);

            const TComponent& sourceCmp = *get(sourceId);

            return target.emplace(targetId, sourceCmp) != nullptr;
        }
    }

    /**
     * @copydoc SparseSetBase::resetTo
     */
    [[nodiscard]] bool
    resetTo(const EntityId targetId, SparseSetBase& sourceSparseSet, const EntityId sourceId) override {

        if constexpr (!std::is_copy_assignable_v<TComponent>) {
            assert(false && "cannot copy the component, is this intentional?");
            return false;
        } else {

            auto& source = static_cast<SparseSet<TComponent>&>(sourceSparseSet);

            auto* sourceCmp = source.get(sourceId);
            if (sourceCmp == nullptr) {
                assert(false && "cannot reset the component, source is not present");
                return false;
            }

            storage_[sparse_[targetId]] = *sourceCmp;

            return true;
        }
    }

    /**
     * @brief Removes an element via swap-and-pop.
     *
     * @param idx Entity id to remove.
     * @return `true` if the element existed and was removed.
     */
    [[nodiscard]] bool remove(const EntityId idx) override {

        if (idx >= sparse_.size() || sparse_[idx] == Tombstone) {
            return false;
        }

        const auto denseIndex = sparse_[idx];
        const auto sparseIdx = denseToSparse_[denseIndex];

        assert(sparseIdx == idx && "Sparse index mismatch");

        if (denseIndex != storage_.size() - 1) {
            storage_[denseIndex] = std::move(storage_.back());
            const auto newSparseIndex = denseToSparse_.back();
            sparse_[newSparseIndex] = denseIndex;
            denseToSparse_[denseIndex] = newSparseIndex;
        }

        storage_.pop_back();
        denseToSparse_.pop_back();

        sparse_[idx] = Tombstone;

        if (denseToSparse_.empty()) {
            maxEntityId_ = Tombstone;
            invalidatedMaxEntityId_ = Tombstone;
        } else if (invalidatedMaxEntityId_ == Tombstone && idx == maxEntityId_) {
            invalidatedMaxEntityId_ = maxEntityId_;
            maxEntityId_ = Tombstone;
        }

        return true;
    }

    /**
     * @brief Retrieves the element at the given index.
     *
     * @param idx The EntityId to look up.
     *
     * @return Pointer to the element, or `nullptr` if not found.
     */
    [[nodiscard]] TComponent* get(const EntityId idx) {

        if (idx >= sparse_.size() || sparse_[idx] == Tombstone) {
            return nullptr;
        }

        return &storage_[sparse_[idx]];
    }

    /**
     * @brief Retrieves the element at the given index.
     *
     * @param idx The EntityId to look up.
     *
     * @return Const pointer to the element, or `nullptr` if not found.
     */
    [[nodiscard]] const TComponent* get(const EntityId idx) const {

        if (idx >= sparse_.size() || sparse_[idx] == Tombstone) {
            return nullptr;
        }

        return &storage_[sparse_[idx]];
    }

    /**
     * @brief Checks whether an element is registered for the specified EntityId.
     *
     * @param idx The EntityId to test.
     *
     * @return True if this sparse set contains the EntityId.
     */
    [[nodiscard]] bool contains(const EntityId idx) const override {
        return idx < sparse_.size() && sparse_[idx] != Tombstone;
    }

    /**
     * @copydoc SparseSetBase::raw
     */
    [[nodiscard]] void* raw(const EntityId id) override {
        TComponent* ptr = get(id);
        return static_cast<void*>(ptr);
    }

    /**
     * @copydoc SparseSetBase::clear
     */
    void clear() override {
        sparse_.clear();
        denseToSparse_.clear();
        storage_.clear();
        maxEntityId_ = Tombstone;
        invalidatedMaxEntityId_ = Tombstone;
    }

    /**
     * @brief Returns a non-owning span of all EntityIds currently in the set.
     *
     * @return A span of EntityIds.
     */
    [[nodiscard]] std::span<const EntityId> entityIds() const noexcept override {
        return denseToSparse_;
    }

    /**
     * @brief Returns the number of components currently stored in this set.
     *
     * @return The number of components.
     */
    [[nodiscard]] size_t componentCount() const noexcept override {
        return storage_.size();
    }

    /**
     * @brief Returns the current maxEntityId of this set, which might equal to Tombstone.
     * In this case this set has currently no valid maxEntityId.
     *
     * @return The max EntityId occuring in this SparseSet.
     */
    [[nodiscard]] EntityId maxEntityId() const noexcept override {
        return maxEntityId_;
    }

    /**
     * @brief Recomputes cached max id if removals invalidated it.
     */
    void finalizeMutations() noexcept override {
        if (invalidatedMaxEntityId_ != Tombstone) {
            maxEntityId_ = denseToSparse_.empty() ? Tombstone : *std::ranges::max_element(denseToSparse_);
            invalidatedMaxEntityId_ = Tombstone;
        }
    }

    /**
     * @brief Forward iterator over dense values and corresponding entity ids.
     */
    struct Iterator {
        using DataIt = std::vector<TComponent>::iterator;
        using IdIt = std::vector<EntityId>::iterator;

        /**
         * @brief Iterator into the dense data storage.
         */
        DataIt dataIt_;

        /**
         * @brief Iterator into the dense-to-sparse ID mapping.
         */
        IdIt idIt_;

        using iterator_category = std::forward_iterator_tag;
        using value_type = TComponent;
        using difference_type = std::ptrdiff_t;
        using pointer = TComponent*;
        using reference = TComponent&;

        Iterator() = default;

        Iterator(DataIt dataIt, IdIt idIt) : dataIt_(dataIt), idIt_(idIt) {}

        reference operator*() const {
            return *dataIt_;
        }
        pointer operator->() const {
            return &*dataIt_;
        }

        /**
         * @brief Returns the `EntityId` of the current element.
         *
         * @return Current `EntityId`.
         */
        [[nodiscard]] EntityId entityId() const {
            return *idIt_;
        }

        [[nodiscard]] bool operator==(const Iterator& other) const {
            return dataIt_ == other.dataIt_;
        }
        [[nodiscard]] bool operator!=(const Iterator& other) const {
            return dataIt_ != other.dataIt_;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        Iterator& operator++() {
            ++dataIt_;
            ++idIt_;
            return *this;
        }
    };

    /**
     * @brief Const forward iterator over dense values and entity ids.
     */
    struct ConstIterator {
        using DataIt = std::vector<TComponent>::const_iterator;
        using IdIt = std::vector<EntityId>::const_iterator;

        /**
         * @brief Const iterator into the dense data storage.
         */
        DataIt dataIt_;

        /**
         * @brief Const iterator into the dense-to-sparse ID mapping.
         */
        IdIt idIt_;

        using iterator_category = std::forward_iterator_tag;
        using value_type = TComponent;
        using difference_type = std::ptrdiff_t;
        using pointer = const TComponent*;
        using reference = const TComponent&;

        ConstIterator() = default;

        ConstIterator(DataIt dataIt, IdIt idIt) : dataIt_(dataIt), idIt_(idIt) {}

        reference operator*() const {
            return *dataIt_;
        }
        pointer operator->() const {
            return &*dataIt_;
        }

        /**
         * @brief Returns the `EntityId` of the current element.
         *
         * @return Current `EntityId`.
         */
        [[nodiscard]] EntityId entityId() const {
            return *idIt_;
        }

        [[nodiscard]] bool operator==(const ConstIterator& other) const {
            return dataIt_ == other.dataIt_;
        }
        [[nodiscard]] bool operator!=(const ConstIterator& other) const {
            return dataIt_ != other.dataIt_;
        }

        ConstIterator operator++(int) {
            ConstIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        ConstIterator& operator++() {
            ++dataIt_;
            ++idIt_;
            return *this;
        }
    };

    /**
     * @brief Returns an iterator to the beginning of the dense storage.
     *
     * @return Iterator pointing to the first element.
     */
    [[nodiscard]] Iterator begin() {
        return Iterator(storage_.begin(), denseToSparse_.begin());
    }

    /**
     * @brief Returns an iterator to the end of the dense storage.
     *
     * @return Iterator pointing past the last element.
     */
    [[nodiscard]] Iterator end() {
        return Iterator(storage_.end(), denseToSparse_.end());
    }

    /**
     * @brief Returns a const iterator to the beginning of the dense storage.
     *
     * @return ConstIterator pointing to the first element.
     */
    [[nodiscard]] ConstIterator begin() const {
        return ConstIterator(storage_.begin(), denseToSparse_.begin());
    }

    /**
     * @brief Returns a const iterator to the end of the dense storage.
     *
     * @return ConstIterator pointing past the last element.
     */
    [[nodiscard]] ConstIterator end() const {
        return ConstIterator(storage_.end(), denseToSparse_.end());
    }
};

} // namespace helios::ecs::storage
