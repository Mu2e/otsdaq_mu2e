# Project Overview

This repository is part of the Mu2e TDAQ suite, based on the artdaq and otsdaq data acquisition frameworks.

## Libraries and Frameworks

otsdaq-mu2e is built using CMake and the Spack package manager. A build environment is provided in the `docker.io/eflumerf/mu2e-spack` Docker image, which includes all necessary dependencies and tools for building. The build environment is located at /opt/mu2edaq within the container.

## Coding Standards

All C++ source files must be formatted with `clang-format`. C/C++ header files should be formatted with `clang-format` using the `.clang-format-hpp` instruction file if it exists, `.clang-format` otherwise. All python files must be formatted with `black`. The project uses the Google C++ Style Guide as a reference for coding standards, with some modifications to fit the specific needs of the otsdaq framework. Developers are encouraged to follow these guidelines to maintain consistency and readability across the codebase.


## Agent Instructions

When using GitHub Copilot, please ensure that the generated code adheres to the project's coding standards and guidelines. Review the generated code carefully and make any necessary adjustments to ensure it fits well within the existing codebase. Additionally, consider the context of the code being generated and how it interacts with other parts of the project to maintain overall coherence and functionality. Keep changes to a minimum, prefer removing older code and replacing with new and more efficient code. Use modern C++ standards. Be critical of existing code and suggest improvements where possible. Always prioritize readability and maintainability in the generated code.
