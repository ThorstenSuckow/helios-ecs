/**
 * @file SparseSet.ixx
 * @brief Generic sparse set data structure for efficient entity-keyed storage.
 */
module;

#include <cassert>
#include <functional>
#include <vector>
#include <cstddef>
#include <span>

export module helios.ecs.SparseSet;

import helios.ecs.types.EntityHandle;
import helios.ecs.types;


using namespace helios::ecs::types;
export namespace helios::ecs {

    /**
     * @brief Abstract base class for type-erased, non thread-safe sparse set access.
     *
     * `SparseSetBase` provides a non-templated interface for polymorphic
     * operations on sparse sets. This enables containers to store
     * heterogeneous pools and perform common operations (e.g., removal)
     * without knowing the concrete element type.
     *
     * @see SparseSet
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
         * @warning Not Thread safe, requires exlusive access to an instance of this class.
         */
        virtual void finalizeMutations() noexcept = 0;
    };


    /**
     * @brief Sentinel value indicating an empty slot in the sparse array.
     *
     * Aliased from `helios::ecs::types::EntityTombstone`.
     */
    constexpr auto Tombstone = EntityTombstone;


    /**
     * @brief A generic sparse set providing O(1) insertion, lookup, and removal.
     *
     * `SparseSet` implements the sparse set data structure pattern, commonly used
     * in Entity Component Systems (ECS) for efficient storage. It maps `EntityId`
     * indices to densely packed data of type `T`.
     *
     * ## Data Structure
     *
     * ```
     * SPARSE ARRAY (indexed by EntityId)
     * [  2  |  x  |  0  |  x  |  1  |  x  |  x  ]   (dense idx)
     *
     * DENSE STORAGE (contiguous)
     * [  T[0] (id=2)  |  T[1] (id=4)  |  T[2] (id=0)  ]
     *
     * DENSE-TO-SPARSE (reverse mapping for swap-and-pop)
     * [  2  |  4  |  0  ]   (EntityId)
     *
     * x = Tombstone (empty slot)
     * ```
     *
     * ## Complexity
     *
     * | Operation | Time    | Space        |
     * |-----------|---------|--------------|
     * | emplace   | O(1)*   | O(max_id)    |
     * | insert    | O(1)*   | O(max_id)    |
     * | get       | O(1)    | -            |
     * | remove    | O(1)    | -            |
     *
     * *Amortized due to potential sparse array resize.
     *
     * @tparam T             The type of elements stored in the set. Must be move-assignable.
     *
     * @see SparseSetBase
     * @see EntityId
     * @see EntityTombstone
     */
    template <typename T>
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
        std::vector<T> storage_;

        /**
         * @brief Max EntityId available in this SparseSet, defaults to `Tombstone`.
         */
        mutable EntityId maxEntityId_ = Tombstone;

        /**
         * @brief Holds the last maxEntityId-value before removal happend.
         * Used to compare with new maxEntityIds: If a new maxEntityId is higher than this value,
         * no maxEntityId needs to be recalculated in finalizeMutations().
         */
        EntityId invalidatedMaxEntityId_ = Tombstone;

        /**
         * @brief Helper for updating Max EntityId.
         *
         * Keeps a possible maxEntityId_-Tombstone as long valid as a invalidatedMaxEntityId_
         * couldnt be resolved.
         *
         * @param idx New EntityId.
         */
        inline void updateMaxEntityId(const EntityId idx) {
            if (invalidatedMaxEntityId_ != Tombstone && invalidatedMaxEntityId_ <= idx) {
                invalidatedMaxEntityId_ = Tombstone;
                maxEntityId_ = idx;
                return;
            }
            
            maxEntityId_ = maxEntityId_ == Tombstone ? idx : std::max(maxEntityId_, idx);
        }
    public:


        using Component_type = T;


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
            sparse_.reserve(capacity);
            storage_.reserve(capacity);
            denseToSparse_.reserve(capacity);
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

        ComponentTypeId<T> componentTypeId() {
            return ComponentTypeId<typename T::Handle_type>::template id<T>();
        };

        /**
         * @brief Constructs and inserts an element at the given index.
         *
         * Forwards arguments to construct `T` in-place.
         *
         * @tparam Args Constructor argument types.
         *
         * @param idx The EntityId to associate with the element.
         * @param args Arguments forwarded to the `T` constructor.
         *
         * @return Pointer to the inserted element, or `nullptr` if the index is already occupied.
         */
        template <typename... Args>
        [[nodiscard]] T* emplace(const EntityId idx, Args&& ...args) {

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
        [[nodiscard]] T* insert(const EntityId idx, T&& obj) {

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
         * @copydoc SparseSetBase::copy
         */
        [[nodiscard]] void* copy(const EntityId sourceId, const EntityId targetId) override {

            if constexpr (!std::copy_constructible<T>) {
                return nullptr;
            } else {
                // not existing
                if (sourceId >= sparse_.size() || sparse_[sourceId] == Tombstone) {
                    return nullptr;
                }

                // already in use
                if (targetId < sparse_.size() && sparse_[targetId] != Tombstone) {
                    return nullptr;
                }

                if (targetId >= sparse_.size()) {
                    sparse_.resize(targetId + 1, Tombstone);
                }

                const auto denseIndex = storage_.size();

                denseToSparse_.push_back(targetId);
                const T& cmp = storage_[sparse_[sourceId]];
                storage_.emplace_back(cmp);

                sparse_[targetId] = denseIndex;

                updateMaxEntityId(targetId);

                return &storage_.back();
            }
        }

        /**
         * @brief Removes the element at the given index using swap-and-pop.
         *
         * @details Uses the swap-and-pop technique for O(1) removal:
         * 1. Move the last element to the position of the removed element
         * 2. Update the sparse array entry for the moved element
         * 3. Pop the last element from dense storage
         * 4. Mark the removed slot as Tombstone
         *
         * @param idx The EntityId of the element to remove.
         *
         * @return True if the element was removed, false if not found.
         */
        [[nodiscard]] bool remove(const EntityId idx) override {

            if (idx >= sparse_.size() || sparse_[idx] == Tombstone) {
                return false;
            }

            const auto denseIndex = sparse_[idx];
            const auto sparseIdx  = denseToSparse_[denseIndex];

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
        [[nodiscard]] T* get(const EntityId idx) {

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
        [[nodiscard]] const T* get(const EntityId idx) const  {

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
            T* ptr = get(id);
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
         * @copydocs SparseSetBase::finalizeMutations()
         */
        void finalizeMutations() noexcept override {
            if (invalidatedMaxEntityId_ != Tombstone) {
                maxEntityId_ = denseToSparse_.empty()
                            ? Tombstone
                            : *std::max_element(denseToSparse_.begin(), denseToSparse_.end());
                invalidatedMaxEntityId_ = Tombstone;
            }
        }

        /**
         * @brief Forward iterator for traversing the sparse set.
         *
         * @details Iterates over the dense storage array, providing access to both
         * the stored element and its associated EntityId.
         */
        struct Iterator {
            using DataIt = typename std::vector<T>::iterator;
            using IdIt = typename std::vector<EntityId>::iterator;

            /**
             * @brief Iterator into the dense data storage.
             */
            DataIt dataIt_;

            /**
             * @brief Iterator into the dense-to-sparse ID mapping.
             */
            IdIt idIt_;

            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = T*;
            using reference = T&;

            Iterator() = default;

            Iterator(DataIt dataIt, IdIt idIt) : dataIt_(dataIt), idIt_(idIt) {}

            reference operator*() const { return *dataIt_; }
            pointer operator->() const { return &*dataIt_; }

            /**
             * @brief Returns the EntityId for the current element.
             *
             * @return The EntityId associated with the current element.
             */
            [[nodiscard]] EntityId entityId() const { return *idIt_; }

            [[nodiscard]] bool operator==(const Iterator& other) const { return dataIt_ == other.dataIt_;}
            [[nodiscard]] bool operator!=(const Iterator& other) const { return dataIt_ != other.dataIt_;}

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            Iterator& operator++() {
                ++dataIt_; ++idIt_; return *this;
            }
        };

        /**
         * @brief Const forward iterator for traversing the sparse set.
         *
         * @details Provides read-only access to elements and their EntityIds.
         */
        struct ConstIterator {
            using DataIt = typename std::vector<T>::const_iterator;
            using IdIt = typename std::vector<EntityId>::const_iterator;

            /**
             * @brief Const iterator into the dense data storage.
             */
            DataIt dataIt_;

            /**
             * @brief Const iterator into the dense-to-sparse ID mapping.
             */
            IdIt idIt_;

            using iterator_category = std::forward_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T*;
            using reference = const T&;

            ConstIterator() = default;

            ConstIterator(DataIt dataIt, IdIt idIt) : dataIt_(dataIt), idIt_(idIt) {}

            reference operator*() const { return *dataIt_; }
            pointer operator->() const { return &*dataIt_; }

            /**
             * @brief Returns the EntityId for the current element.
             *
             * @return The EntityId associated with the current element.
             */
            [[nodiscard]] EntityId entityId() const { return *idIt_; }

            [[nodiscard]] bool operator==(const ConstIterator& other) const { return dataIt_ == other.dataIt_;}
            [[nodiscard]] bool operator!=(const ConstIterator& other) const { return dataIt_ != other.dataIt_;}

            ConstIterator operator++(int) {
                ConstIterator tmp = *this;
                ++(*this);
                return tmp;
            }

            ConstIterator& operator++() {
                ++dataIt_; ++idIt_; return *this;
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



}
