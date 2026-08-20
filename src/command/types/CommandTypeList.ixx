/**
 * @file CommandTypeLIst.ixx
 * @brief Helper struct for maintaining list of Command-types.
 */
module;


export module helios.ecs.command.types:CommandTypeList;

export namespace helios::ecs::command::types {


    template<typename ... TCommands>
    struct CommandTypeList{};


};