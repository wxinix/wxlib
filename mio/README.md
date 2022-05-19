# About

This repository is maintained by Wuping Xin.

Forked from https://github.com/mandreyel/mio, who created this well-documented and easy-to-use original library in modern C++. I love this code, and thank the original author mandreyel for his great work.

The motivation for this fork is to have a cross-platform header-only memory mapped file IO lib for my C++17/C++2x projects. It serves as my own "dogfooding" project to try out latest and future experimental C++ 2x features.  

The following changes have been made: 

- "Beautified" single header; duplicate license headers and messy macros from amalgamation are removed
- Internal clean-up and reformatting for a (maybe slightly) neater C++ style
- Added a new check that the requested bytes (i.e., offset + length) must not exceed address space limit (e.g. 2GB on x86)
- Eliminated direct pointer arithmetics
- Consistent usage of size_type throughout the code
- Added StringReader class, which provides better performance than std::getline (x10 ~ x15 faster), and support both async and sync loading, and event-based handling
- Added new classes and templates for processing CSV files, using C++20 meta-template programming, that support declarative style csv file processing
- Some other minor bug fix(es)

Note: This repository requires C++ 20 (mainly due to the usage of std::span and concept throughout). The following is adapted from the original README about its usage.

# MIO
An easy to use header-only cross-platform C++20 memory mapping library with an MIT license.

mio has been created with the goal to be easily included (i.e. no dependencies) in any C++ project that needs memory mapped file IO without the need to pull in Boost.

Please feel free to open an issue, I'll try to address any concerns as best I can.

### Why?
Because memory mapping is the best thing since sliced bread! More seriously, the primary motivation for writing this library instead of using Boost.Iostreams, was the lack of support for establishing a memory mapping with an already open file handle/descriptor. This is possible with mio.

Furthermore, Boost.Iostreams' solution requires that the user pick offsets exactly at page boundaries, which is cumbersome and error-prone. mio, on the other hand, manages this internally, accepting any offset and finding the nearest page boundary. Albeit a minor nitpick, Boost.Iostreams implements memory mapped file IO with a `std::shared_ptr` to provide shared semantics, even if not needed, and the overhead of the heap allocation may be unnecessary and/or unwanted.

In mio, there are two classes to cover the two use-cases: one that is move-only (basically a zero-cost abstraction over the system specific memory mapping functions), and the other that acts just like its Boost.Iostreams counterpart, with shared semantics.

### How to create a mapping
NOTE: the file must exist before creating a mapping.

There are three ways to map a file into memory:

- Using the constructor, which throws a `std::system_error` on failure:
```c++
mio::mmap_source mmap(path, offset, size_to_map);
```
or you can omit the `offset` and `size_to_map` arguments, in which case the
entire file is mapped:
```c++
mio::mmap_source mmap(path);
```

- Using the factory function:
```c++
std::error_code error;
mio::mmap_source mmap = mio::make_mmap_source(path, offset, size_to_map, error);
```
or:
```c++
mio::mmap_source mmap = mio::make_mmap_source(path, error);
```

- Using the `map` member function:
```c++
std::error_code error;
mio::mmap_source mmap;
mmap.map(path, offset, size_to_map, error);
```
or:
```c++
mmap.map(path, error);
```
**NOTE:** The constructors **require** exceptions to be enabled. If you prefer
to build your projects with `-fno-exceptions`, you can still use the other ways.

Moreover, in each case, you can provide either some string type for the file's path, or you can use an existing, valid file handle.
```c++
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <mio/mmap.hpp>
#include <algorithm>

int main()
{
    // NOTE: error handling omitted for brevity.
    const int fd = open("file.txt", O_RDONLY);
    mio::mmap_source mmap(fd, 0, mio::map_entire_file);
    // ...
}
```
However, mio does not check whether the provided file descriptor has the same access permissions as the desired mapping, so the mapping may fail. Such errors are reported via the `std::error_code` out parameter that is passed to the mapping function.

**WINDOWS USERS**: This library *does* support the use of wide character types
for functions where character strings are expected (e.g. path parameters).

### Example

```c++
#include <mio/mmap.hpp>
#include <system_error> // for std::error_code
#include <cstdio> // for std::printf
#include <cassert>
#include <algorithm>
#include <fstream>

int handle_error(const std::error_code& error);
void allocate_file(const std::string& path, const int size);

int main()
{
    const auto path = "file.txt";

    // NOTE: mio does *not* create the file for you if it doesn't exist! You
    // must ensure that the file exists before establishing a mapping. It
    // must also be non-empty. So for illustrative purposes the file is
    // created now.
    allocate_file(path, 155);

    // Read-write memory map the whole file by using `map_entire_file` where the
    // length of the mapping is otherwise expected, with the factory method.
    std::error_code error;
    mio::mmap_sink rw_mmap = mio::make_mmap_sink(
            path, 0, mio::map_entire_file, error);
    if (error) { return handle_error(error); }

    // You can use any iterator based function.
    std::fill(rw_mmap.begin(), rw_mmap.end(), 'a');

    // Or manually iterate through the mapped region just as if it were any other 
    // container, and change each byte's value (since this is a read-write mapping).
    for (auto& b : rw_mmap) {
        b += 10;
    }

    // Or just change one value with the subscript operator.
    const int answer_index = rw_mmap.size() / 2;
    rw_mmap[answer_index] = 42;

    // Don't forget to flush changes to disk before unmapping. However, if
    // `rw_mmap` were to go out of scope at this point, the destructor would also
    // automatically invoke `sync` before `unmap`.
    rw_mmap.sync(error);
    if (error) { return handle_error(error); }

    // We can then remove the mapping, after which rw_mmap will be in a default
    // constructed state, i.e. this and the above call to `sync` have the same
    // effect as if the destructor had been invoked.
    rw_mmap.unmap();

    // Now create the same mapping, but in read-only mode. Note that calling the
    // overload without the offset and file length parameters maps the entire
    // file.
    mio::mmap_source ro_mmap;
    ro_mmap.map(path, error);
    if (error) { return handle_error(error); }

    const int the_answer_to_everything = ro_mmap[answer_index];
    assert(the_answer_to_everything == 42);
}

int handle_error(const std::error_code& error)
{
    const auto& errmsg = error.message();
    std::printf("error mapping file: %s, exiting...\n", errmsg.c_str());
    return error.value();
}

void allocate_file(const std::string& path, const int size)
{
    std::ofstream file(path);
    std::string s(size, '0');
    file << s;
}
```

`mio::basic_mmap` is move-only, but if multiple copies to the same mapping are needed, use `mio::basic_shared_mmap` which has `std::shared_ptr` semantics and has the same interface as `mio::basic_mmap`.
```c++
#include <mio/shared_mmap.hpp>

mio::shared_mmap_source shared_mmap1("path", offset, size_to_map);
mio::shared_mmap_source shared_mmap2(std::move(mmap1)); // or use operator=
mio::shared_mmap_source shared_mmap3(std::make_shared<mio::mmap_source>(mmap1)); // or use operator=
mio::shared_mmap_source shared_mmap4;
shared_mmap4.map("path", offset, size_to_map, error);
```

It's possible to define the type of byte (which has to be the same width as `char`), though aliases for the most common ones are provided by default:
```c++
using mmap_source = basic_mmap_source<char>;
using ummap_source = basic_mmap_source<unsigned char>;

using mmap_sink = basic_mmap_sink<char>;
using ummap_sink = basic_mmap_sink<unsigned char>;
```
But it may be useful to define your own types, say when using the new `std::byte` type in C++17:
```c++
using mmap_source = mio::basic_mmap_source<std::byte>;
using mmap_sink = mio::basic_mmap_sink<std::byte>;
```

Though generally not needed, since mio maps users requested offsets to page boundaries, you can query the underlying system's page allocation granularity by invoking `mio::page_size()`, which is located in `mio/page.hpp`.

### Single Header File 
Mio can be added to your project as a single header file simply by including `\mio\mio.hpp`. 

---
Last edited by Wuping Xin (@wxinix).