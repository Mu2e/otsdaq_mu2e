# Project Overview

This repository is part of the Mu2e TDAQ suite, based on the artdaq and otsdaq data acquisition frameworks.

## Libraries and Frameworks

otsdaq-mu2e is built using CMake and the Spack package manager. A build environment is provided in the `docker.io/eflumerf/mu2e-spack` Docker image, which includes all necessary dependencies and tools for building. The build environment is located at /opt/mu2edaq within the container.

## Coding Standards

All C++ source files must be formatted with `clang-format`. All python files must be formatted with `black`. The project uses the Google C++ Style Guide as a reference for coding standards, with some modifications to fit the specific needs of the otsdaq framework. Developers are encouraged to follow these guidelines to maintain consistency and readability across the codebase.
