/**
 * @file CommandBufferFactory.ixx
 * @brief Factory for creating command buffers based on command type lists.
 */
module;


export module helios.ecs.command.CommandBufferFactory;

import helios.ecs.command.types;
import helios.ecs.command.NullCommandBuffer;

export namespace helios::ecs::command {

    template<
        template <typename...> typename TCommandBuffer,
        typename TInitContext,
        typename TFlushContext
    >
    class CommandBufferFactory {
        public:

            using InitContextType = TInitContext;
            using FlushContextType = TFlushContext;

            template<typename ... TCommands>
            [[nodiscard]] static auto make(types::CommandTypeList<TCommands...>) {

                if constexpr (sizeof...(TCommands) == 0) {
                    return NullCommandBuffer{};
                } else {
                    return TCommandBuffer<TCommands...>{};
                }

            }
        };

};