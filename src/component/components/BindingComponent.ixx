module;

#include <concepts>

export module helios.ecs.component.components:BindingComponent;

export namespace helios::ecs::components {

/**
 * @brief Generic component that stores a handle reference to another entity.
 *
 * @tparam TOwnerHandle Handle type of the entity owning this component.
 * @tparam TTargetHandle Handle type of the referenced target entity.
 * @tparam TDomainTag Optional tag type for domain-specific specialization.
 */
template <typename TOwnerHandle, typename TTargetHandle, typename TDomainTag>
class BindingComponent {

    TTargetHandle targetHandle_{};

public:
    using HandleType = TOwnerHandle;

    /**
     * @brief Creates a binding from an explicit target handle.
     *
     * @param targetHandle Handle of the referenced target entity.
     */
    explicit BindingComponent(const TTargetHandle targetHandle) : targetHandle_(targetHandle) {};

    /**
     * @brief Creates a binding from a target entity instance.
     *
     * @tparam TTargetEntity Entity type exposing `HandleType` and `handle()`.
     * @param targetEntity Referenced target entity.
     */
    template <typename TTargetEntity>
        requires std::same_as<TTargetHandle, typename TTargetEntity::HandleType>
    explicit BindingComponent(const TTargetEntity targetEntity) : targetHandle_(targetEntity.handle()){};

    /**
     * @brief Returns the bound target handle.
     *
     * @return Handle of the referenced target entity.
     */
    [[nodiscard]] TTargetHandle targetHandle() const noexcept {
        return targetHandle_;
    }
};
} // namespace helios::ecs::components