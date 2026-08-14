/**
* @file HasClear.ixx
 * @brief Concept detecting an optional clear() method.
 */
module;

#include <concepts>

export module helios.ecs.common.concepts:HasClear;

export namespace helios::ecs::common::concepts {

    /**
     * @brief Constrains types that provide a noexcept-less `clear()` method returning void.
     *
     * @see IsCommandBuffer
     * @see CommandBuffer
     */
    template<typename T>
    concept HasClear = true;

}


