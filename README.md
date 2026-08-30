# helios::ecs

Generic ECS primitives for the helios module ecosystem.

## Overview

`helios::ecs` provides reusable C++23 building blocks for entity identity,
component storage, lifecycle reflection, and typed queries. It is used by
higher-level [helios](https://github.com/thorstensuckow/helios-engine) modules that need strongly typed entity domains and compact
component storage and can be used as a standalone ECS-implementation with other projects that require high performance for real-time applications.

## Features

- Versioned, strongly typed entity handles via `EntityHandle<TStrongId>`
- Configurable entity registries with strong-id collision tracking
- Sparse-set component storage with dense iteration
- Typed entity/component views with filtering helpers
- Trait-based component lifecycle reflection
- Multi-domain world composition with `TypedHandleWorld`

## Module surface

| Area | Public API |
|------|------------|
| Entity management | `EntityHandle`, `EntityRegistry`, `EntityManager`, `Entity`, `EntityResolver`, `TypedHandleWorld` |
| Component metadata | `ComponentTypeId`, `ComponentOps`, `ComponentOpsRegistry`, `ComponentReflector` |
| Storage/query | `SparseSet`, `View` |
| Lookup strategies | `HashedLookupStrategy`, `LinearLookupStrategy` |
| Concepts/traits | ECS constraints and lifecycle hook detection traits |

## Usage

### C++ module

```cpp
import helios.ecs;
```

### CMake

When used as a subdirectory, link against the exported target:

```cmake
add_subdirectory(path/to/helios-ecs)
target_link_libraries(your_target PRIVATE helios::ecs)
```

## Development

Build the project:

```bash
cmake -S . -B build
cmake --build build
```

Quick devtools entrypoint from this repository:

```bash
sh ./run-devtools.sh format
sh ./run-devtools.sh format-fix
sh ./run-devtools.sh tidy
sh ./run-devtools.sh tidy-fix
```

`run-devtools.sh` is a thin wrapper around CMake targets and expects an already configured build directory (default: `cmake-build-debug`).

Run formatting checks:

```bash
cmake --build cmake-build-debug --target format
```

Run formatting with in-place fixes:

```bash
cmake --build cmake-build-debug --target format-fix
```

Run clang-tidy checks:

```bash
cmake --build cmake-build-debug --target tidy
```

Run clang-tidy with autofix:

```bash
cmake --build cmake-build-debug --target tidy-fix
```

Target-specific variants are also available:

```bash
cmake --build cmake-build-debug --target format-helios_ecs
cmake --build cmake-build-debug --target tidy-helios_ecs
```

Formatting and clang-tidy checks are sourced from shared `helios-devtools` config.

Run tests when test discovery is enabled:

```bash
ctest --test-dir build --output-on-failure
```

## Related repositories

- [`helios-engine`](https://github.com/thorstensuckow/helios-engine)
- [`helios-math`](https://github.com/thorstensuckow/helios-math)
- [`helios-opengl`](https://github.com/thorstensuckow/helios-opengl)
- [`helios-glfw`](https://github.com/thorstensuckow/helios-glfw)
