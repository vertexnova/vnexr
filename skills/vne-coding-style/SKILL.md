---
name: vne-coding-style
description: >
  Enforce VertexNova C++ and Objective-C++ conventions (naming, formatting,
  initialization, modern C++). Use this skill when writing or modifying any
  code under src/, include/, samples/, or tests/ in vnexr.
---

# VertexNova Coding Style

A compact checklist for VertexNova code. The full rules with rationale live in
[CODING_GUIDELINES.md](../../CODING_GUIDELINES.md); this skill is the quick
reference to apply while editing. C++ standard is C++20.

## 1. Naming (apply exactly)

| Construct | Style | Example |
|-----------|-------|---------|
| Classes / Structs | PascalCase | `Buffer`, `ShaderCompiler` |
| Interface classes | `I` + PascalCase | `IRenderer`, `IBuffer` |
| Enums | PascalCase | `LogSink`, `ShaderStage` |
| Enum values | `e` + PascalCase, explicit value | `eNone = 0`, `eConsole = 1` |
| Type aliases | PascalCase, no `T` prefix | `BufferHandle`, `EntityId` |
| Functions / Methods | camelCase | `initialize()`, `createBuffer()` |
| Constants | `k` + PascalCase | `kMaxBufferSize` |
| Private / Protected members | snake_case + `_` | `buffer_size_`, `is_initialized_` |
| Public members | snake_case | `buffer_size` |
| Locals / Parameters | snake_case | `buffer_size`, `usage` |
| Static (private) | `s_` + snake_case + `_` | `s_instance_count_` |
| Global | `g_` + snake_case | `g_device` |
| Booleans | `is_` / `has_` / `can_` / `should_` | `is_ready_`, `has_alpha_` |
| Macros | ALL_CAPS, `VNE_` prefix | `VNE_ASSERT` |
| Namespaces | lowercase | `vne`, `vne::xr`, `vne::xr::samples` |
| File names | snake_case | `shader_compiler.h` |

## 2. Files and headers

- `#pragma once` for every header (never include guards).
- Start each file with the project copyright banner block (see any existing
  header, e.g. the block in the Header File Structure section of the guideline).
- Include ordering and self-containment: see [vne-header-hygiene](../vne-header-hygiene/SKILL.md).
  `.clang-format` sets `SortIncludes: false`, so include order is hand-maintained.

## 3. Types and members

- `enum class` only, `e`-prefixed values with explicit integer values.
- Rule of Zero first; when a special member is needed, declare the full Rule of Five.
- `explicit` on single-argument constructors.
- Prefer brace initialization `{}`; give members in-class defaults
  (`bool is_mapped_{false};`) and use the initializer list only for injected values.
- Struct = data container; class = invariants plus behavior. Order members:
  public types/constants, constructors/destructor, public methods, protected, private.

## 4. Functions

- Return values over out-parameters; `[[nodiscard]]` when the result must be used.
- `const` on methods that do not mutate; `noexcept` on non-throwing methods and moves.
- References for non-nullable parameters, pointers only when null is meaningful.

## 5. Modern C++ to prefer

- `std::optional<T>` for "may be absent", `std::string_view` for non-owning string
  params, `std::span<T>` for non-owning array views.
- `nullptr` (never `NULL`/`0`), `constexpr` / `if constexpr`, structured bindings,
  range-based `for`, `auto` only when the type is obvious.
- Smart pointers for ownership (`std::unique_ptr` exclusive, `std::shared_ptr` shared,
  `std::weak_ptr` to break cycles); RAII for every resource.

## 6. Objective-C++

- C++ interface in `.h`, implementation in `.mm`. Import order: `#include` C++ headers,
  then `#import` Objective-C headers, then system frameworks. ARC is on; no manual
  `retain`/`release`.

## 7. Documentation and prose

- Doxygen `@brief` / `@param` / `@return` on public APIs whose behavior is not obvious.
- Document thread safety with `@threadsafe` or `@warning Not thread-safe`.
- Keep all comments, identifiers, and docs ASCII-only and free of AI phrasing:
  see [plain-ascii-authoring](../plain-ascii-authoring/SKILL.md).

## 8. Before you finish

Run formatting, the build, and the tests: see
[vne-build-verify](../vne-build-verify/SKILL.md). Formatting and static analysis are
CI-enforced via `.clang-format` and `.clang-tidy`, so do not hand-format. A change
to a rendering path is not verified until it has run on every backend the platform
supports.

Source of truth: [CODING_GUIDELINES.md](../../CODING_GUIDELINES.md).
