/**
 * @file _module.ixx
 * @brief Module aggregator for helios.ecs.
 */

export module helios.ecs;


export import helios.ecs.common.concepts;
export import helios.ecs.common.types;
export import helios.ecs.storage;
export import helios.ecs.command;
export import helios.ecs.component;
export import helios.ecs.manager;
export import helios.ecs.system;

export import helios.ecs.EcsWorld;
export import helios.ecs.Entity;
export import helios.ecs.EntitySpace;
export import helios.ecs.EntityManager;
export import helios.ecs.EntityRegistry;
export import helios.ecs.EntityAccessor;
export import helios.ecs.TypedHandleWorld;
export import helios.ecs.View;
