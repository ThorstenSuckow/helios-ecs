/**
 * @file _module.ixx
 * @brief Aggregate module re-exporting all engine role tags under runtime.world.
 */

export module helios.ecs.system;

export import helios.ecs.system.System;
export import helios.ecs.system.SystemRegistry;
export import helios.ecs.system.CallableSystem;

export import helios.ecs.system.tags;
export import helios.ecs.system.types;
export import helios.ecs.system.concepts;