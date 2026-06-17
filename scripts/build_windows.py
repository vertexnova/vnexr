#!/usr/bin/env python3
"""
VneXR Windows Build Script
Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
Licensed under the Apache License, Version 2.0 (the "License")

This script builds VneXR for Windows with Visual Studio integration.
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import List, Optional


class BuildConfig:
    def __init__(self):
        self.jobs = 10
        self.build_type = "Debug"
        self.action = "configure_and_build"
        self.lib_type = "shared"
        self.with_dev = False
        self.with_tests = False
        self.with_examples = False
        self.clean_build = False
        self.interactive = False
        self.platform = "Windows"
        self.compiler = "cl"


def find_visual_studio() -> Optional[str]:
    vs_paths = [
        r"C:\Program Files\Microsoft Visual Studio\2022\Community",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2022\Community",
        r"C:\Program Files\Microsoft Visual Studio\2019\Community",
        r"C:\Program Files\Microsoft Visual Studio\2019\Professional",
    ]
    for vs_path in vs_paths:
        if os.path.exists(vs_path):
            return vs_path
    return None


def get_compiler_version() -> str:
    try:
        result = subprocess.run(["cl"], capture_output=True, text=True, stderr=subprocess.STDOUT)
        match = re.search(r"Version (\d+\.\d+\.\d+)", result.stdout or "")
        if match:
            return match.group(1)
    except (subprocess.CalledProcessError, FileNotFoundError):
        pass
    vs_path = find_visual_studio()
    if vs_path:
        if "2022" in vs_path:
            return "2022"
        if "2019" in vs_path:
            return "2019"
    return "unknown"


def check_visual_studio_env() -> bool:
    try:
        subprocess.run(["cl"], capture_output=True, check=False)
        return True
    except FileNotFoundError:
        return False


def setup_visual_studio_env():
    vs_path = find_visual_studio()
    if not vs_path:
        print("Error: Visual Studio not found. Please install VS 2019 or 2022.")
        sys.exit(1)
    vsdevcmd = os.path.join(vs_path, "Common7", "Tools", "VsDevCmd.bat")
    if not os.path.exists(vsdevcmd):
        print(f"Error: VsDevCmd.bat not found at {vsdevcmd}")
        sys.exit(1)
    print(f"Found Visual Studio at: {vs_path}")
    print("Please run this script from a Visual Studio Developer Command Prompt")
    print(f"Or run: {vsdevcmd}")
    sys.exit(1)


def check_cmake() -> bool:
    try:
        subprocess.run(["cmake", "--version"], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def interactive_mode(config: BuildConfig):
    print("=== VneXR Interactive Build Configuration ===")
    print(f"Detected Platform: {config.platform}\n")
    print("Select Build Type:\n1) Debug (default)\n2) Release\n3) RelWithDebInfo\n4) MinSizeRel")
    build_choice = input("Enter choice (1-4) [1]: ").strip()
    config.build_type = {"2": "Release", "3": "RelWithDebInfo", "4": "MinSizeRel"}.get(build_choice, "Debug")
    print("\nSelect Action:\n1) Configure only\n2) Configure and build (default)\n3) Configure, build, and test")
    action_choice = input("Enter choice (1-3) [2]: ").strip()
    config.action = {"1": "configure", "3": "test"}.get(action_choice, "configure_and_build")
    if input("\nEnable tests? (y/N): ").strip().lower() == "y":
        config.with_tests = True
    if input("Enable examples? (y/N): ").strip().lower() == "y":
        config.with_examples = True
    if input("\nClean build directory before starting? (y/N): ").strip().lower() == "y":
        config.clean_build = True
    jobs_input = input(f"Number of parallel jobs (default: {config.jobs}): ").strip()
    if jobs_input:
        try:
            config.jobs = int(jobs_input)
        except ValueError:
            pass
    print("\n=== Configuration Summary ===")
    print(
        f"Platform: {config.platform}\nBuild Type: {config.build_type}\n"
        f"Action: {config.action}\nTests: {config.with_tests}\nExamples: {config.with_examples}\n"
        f"Clean: {config.clean_build}\nJobs: {config.jobs}"
    )
    if input("Proceed? (Y/n): ").strip().lower() == "n":
        print("Build cancelled.")
        sys.exit(0)


def cmake_generator(compiler_version: str) -> str:
    """Map detected VS/MSVC version to a CMake -G generator string."""
    version = compiler_version.strip()
    if version in ("16", "2019") or version.startswith("16."):
        return "Visual Studio 16 2019"
    if version in ("17", "2022") or version.startswith("17."):
        return "Visual Studio 17 2022"
    match = re.match(r"19\.(\d+)", version)
    if match:
        if int(match.group(1)) >= 30:
            return "Visual Studio 17 2022"
        return "Visual Studio 16 2019"
    vs_path = find_visual_studio()
    if vs_path:
        if "2019" in vs_path:
            return "Visual Studio 16 2019"
        if "2022" in vs_path:
            return "Visual Studio 17 2022"
    return "Visual Studio 17 2022"


def build_cmake_command(
    project_root: Path,
    build_type: str,
    lib_type: str,
    with_dev: bool,
    with_tests: bool,
    with_examples: bool,
    compiler_version: str,
) -> List[str]:
    dev_flag = "ON" if with_dev else "OFF"
    tests_flag = "ON" if with_tests else "OFF"
    samples_flag = "ON" if with_examples else "OFF"
    generator = cmake_generator(compiler_version)
    return [
        "cmake",
        "-G",
        generator,
        "-A",
        "x64",
        "-DCMAKE_BUILD_TYPE=" + build_type,
        "-DCMAKE_C_COMPILER=cl",
        "-DCMAKE_CXX_COMPILER=cl",
        f"-DVNE_XR_LIB_TYPE={lib_type}",
        f"-DVNE_XR_DEV={dev_flag}",
        f"-DVNE_XR_TESTS={tests_flag}",
        f"-DVNE_XR_EXAMPLES={samples_flag}",
        str(project_root),
    ]


def clean_build_dir(build_dir: Path):
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)


def ensure_build_dir(build_dir: Path):
    build_dir.mkdir(parents=True, exist_ok=True)


def configure_project(
    build_dir: Path,
    project_root: Path,
    config: BuildConfig,
    compiler_version: str,
) -> bool:
    print("Configuring project...")
    generator = cmake_generator(compiler_version)
    print(f"CMake generator: {generator}")
    try:
        subprocess.run(
            build_cmake_command(
                project_root,
                config.build_type,
                config.lib_type,
                config.with_dev,
                config.with_tests,
                config.with_examples,
                compiler_version,
            ),
            cwd=build_dir,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        print("Error: CMake configuration failed")
        return False


def build_project(build_dir: Path, build_type: str, jobs: int) -> bool:
    print(f"Building with {jobs} parallel jobs...")
    try:
        subprocess.run(
            ["cmake", "--build", ".", "--config", build_type, "--parallel", str(jobs)],
            cwd=build_dir,
            check=True,
        )
        return True
    except subprocess.CalledProcessError:
        print("Error: Build failed")
        return False


def run_tests(build_dir: Path, build_type: str) -> bool:
    print("Running tests...")
    try:
        subprocess.run(["ctest", "-C", build_type, "--output-on-failure"], cwd=build_dir, check=True)
        return True
    except subprocess.CalledProcessError:
        print("Warning: Some tests failed")
        return False


def main():
    parser = argparse.ArgumentParser(description="Build VneXR for Windows")
    parser.add_argument("-t", "--build-type", choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"], default="Debug")
    parser.add_argument("-a", "--action", choices=["configure", "build", "configure_and_build", "test"], default="configure_and_build")
    parser.add_argument("-l", "--lib-type", choices=["shared", "static"], default="shared")
    parser.add_argument("--dev", action="store_true", help="Enable tests + examples (VNE_XR_DEV=ON)")
    parser.add_argument("--with-tests", action="store_true", help="Build vnexr_tests")
    parser.add_argument("--with-examples", "--with-samples", action="store_true", help="Build example programs")
    parser.add_argument("--no-tests", action="store_true", help="Omit tests")
    parser.add_argument("--no-examples", "--no-samples", action="store_true", help="Omit examples")
    parser.add_argument("-j", "--jobs", type=int, default=10)
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--interactive", action="store_true")
    args = parser.parse_args()

    config = BuildConfig()
    config.build_type = args.build_type
    config.action = args.action
    config.lib_type = args.lib_type
    config.with_dev = args.dev
    config.with_tests = args.with_tests
    config.with_examples = args.with_examples
    config.jobs = args.jobs
    config.clean_build = args.clean
    config.interactive = args.interactive

    no_tests = args.no_tests
    no_examples = args.no_examples
    if no_tests:
        config.with_tests = False
    if no_examples:
        config.with_examples = False
    if config.with_dev:
        if not no_tests:
            config.with_tests = True
        if not no_examples:
            config.with_examples = True

    if config.interactive:
        interactive_mode(config)

    if not check_visual_studio_env():
        setup_visual_studio_env()
    if not check_cmake():
        print("Error: CMake not found in PATH")
        sys.exit(1)

    compiler_version = get_compiler_version()
    print(f"{config.platform} :: {config.compiler}-{compiler_version}")

    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    build_dir = (
        project_root
        / "build"
        / config.lib_type
        / config.build_type
        / f"build-windows-{config.compiler}-{compiler_version}"
    )

    if config.clean_build:
        clean_build_dir(build_dir)
    else:
        ensure_build_dir(build_dir)

    if config.action == "test" and not config.with_tests:
        print("ERROR: Tests are disabled. Reconfigure with --with-tests or --dev.")
        sys.exit(1)

    if config.action in ["configure", "build", "configure_and_build", "test"]:
        if not configure_project(build_dir, project_root, config, compiler_version):
            sys.exit(1)
    if config.action in ["build", "configure_and_build", "test"]:
        if not build_project(build_dir, config.build_type, config.jobs):
            sys.exit(1)
    if config.action == "test":
        run_tests(build_dir, config.build_type)

    print("\n=== Build completed successfully ===")
    print(f"Build directory: {build_dir}")
    print(f"Tests enabled: {config.with_tests} | Examples enabled: {config.with_examples}")


if __name__ == "__main__":
    main()
