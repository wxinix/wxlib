# wxlib
All-in-one C++20  library for MsgPack, IPC, MemMap, Pattern Matching, and various AVX utilities for high-performance traffic and transportation modelling.

This repository synthesizes and updates several awesome C++11 or C++17 open-source projects - some of them have not been updated for a while, augmented with additional enhancements and updates as a modern C++20 library targeted for *Intelligent Transport Systems (ITS) and transportation modelling applications*.

- serves as a single-point code hub for additional customizations, extensions, and add-ons.  Every single line of those initial C++11 or C++17 projects have been carefully reviewed, then re-organized, refactored, improved, and brought up to date with C++20.

- test cases are migrated to using doctest, while enhancements and additions are implemented where deemed necessary and fit.

- coding adjusted throughout for a consistent style across the projects.

To this end, wxlib may have significantly *changed if not improved* the codebase of the initial projects. You are certainly welcome to use wxlib, which provides many improvements while requiring C++20, or just use each respective original project that are acknowledged below. 

## Acknowledgements

wxlib incorporates (or is inspired by) the following C++ 11 or C++ 17 projects:

- @mikeloomisgg - [CppPack](https://github.com/mikeloomisgg/cppack) modern c++ 17 implementation of the msgpack specification.
- @mandreyel - [mio](https://github.com/mandreyel/mio) cross-platform C++11 header-only library for memory mapped file IO.
- @BowenFu [matchit.cpp](https://github.com/mandreyel/mio) lightweight single-header pattern-matching library for C++17 with macro-free APIs. 
- @mutouyun [cpp-ipc](https://github.com/mutouyun/cpp-ipc) high-performance inter-process communication using shared memory on Linux/Windows.

## MPL/GPL/LGPL License
wxlib adopts an [MPL/GPL/LGPL tri-license](https://github.com/wxinix/wxlib/blob/main/LICENCE.md), permissive for commercial applications, and flexible for non-commercial open-source projects. You are free to use those original C++11 or C++17 projects, while sticking to their respective original license, or use wxlib following [MPL/GPL/LGPL tri-license](https://github.com/wxinix/wxlib/blob/main/LICENCE.md).