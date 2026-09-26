module;

#include <concepts>

export module helios.ecs.component.components:BindingComponent;

export namespace helios::ecs::components {

/**
 * @brief Generic component that stores a handle reference to another entity.
 *
 * @tparam TTargetHandle Handle type of the referenced target entity.
 * @tparam TDomainTag Optional tag type for domain-specific specialization.
 */
template <typename TTargetHandle, typename TDomainTag>
class BindingComponent {

    TTargetHandle targetHandle_{};

public:
    /**
     * @brief Creates a binding from an explicit target handle.
     *
     * @param targetHandle Handle of the referenced target entity.
     */
    explicit BindingComponent(const TTargetHandle targetHandle) : targetHandle_(targetHandle) {};

    /**
     * @brief Creates a binding from a target entity instance.
     *
     * @tparam TTargetEntity Entity type exposing `handle()`.
     * @param targetEntity Referenced target entity.
     */
    template <typename TTargetEntity>
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