---
name: vne-testing
description: >
  Conventions for writing and organizing GoogleTest tests in vnexr, including
  backend-gated tests. Use this skill when adding or changing tests, or when a
  behavior change needs test coverage.
---

# Testing Conventions

vnexr uses GoogleTest. A feature or bug fix ships with a test unless the behavior
can only be observed on a GPU. Tests must be deterministic and fast.

## 1. Layout

- `tests/` - the main suite, one file per unit, built into a single `vnexr_tests`
  binary.
- `tests/backend/{vulkan,metal,webgpu,null,macos}/` - tests that need a specific
  backend.
- `tests/fixtures/` - shared helpers and fixtures.

## 2. File and case naming

- One test file per unit: `<subject>_test.cpp`, matching the header under test
  (`vertex_layout.h` -> `vertex_layout_test.cpp`).
- Follow project naming in test code too (PascalCase types, camelCase methods,
  snake_case locals); see [vne-coding-style](../vne-coding-style/SKILL.md).
- Register new files in [tests/CMakeLists.txt](../../tests/CMakeLists.txt).

## 3. Structure

- Arrange, Act, Assert, with a blank line between phases.
- One behavior per `TEST` / `TEST_F`; give the case a name that states the behavior.
- Keep mocks minimal and assert on observable behavior, not implementation details.

## 4. Determinism and hardware

- No `sleep`, no timeouts, no wall-clock or ordering assumptions.
- Fix random seeds; avoid data races in threaded tests.
- Prefer the null backend for anything that does not need a real device: it
  exercises descriptor validation and resource lifetimes without a GPU.
- A test that needs a real backend belongs under `tests/backend/<backend>/` and must
  skip cleanly (`GTEST_SKIP()`) when the device or extension is unavailable, never
  fail.

## 5. What is worth a test here

Descriptor and reflection logic is testable without a GPU and is where most silent
breakage lives: vertex layout strides and offsets, shader resource set validation,
dynamic-offset bookkeeping, format and capability mapping, swapchain descriptor
contracts. Prefer a null-backend test over a manual visual check when the behavior
can be observed through a descriptor or reflection result.

Rendering output itself has no automated coverage: there is no image-comparison
harness and no headless mode, so a visual change is verified by running the sample
on each backend (see [vne-build-verify](../vne-build-verify/SKILL.md)).

## 6. Running

```bash
ctest --test-dir <build-dir> --output-on-failure
ctest --test-dir <build-dir> -R vnexr_tests
<build-dir>/bin/vnexr_tests --gtest_filter=Suite.TestName
```

Build the test target with `--dev` or `--with-tests`; see
[vne-build-verify](../vne-build-verify/SKILL.md).

Source of truth: [tests/CMakeLists.txt](../../tests/CMakeLists.txt),
[CONTRIBUTING.md](../../CONTRIBUTING.md).
