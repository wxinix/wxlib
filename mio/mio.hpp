/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

/*!
  MIT License
  Copyright (c) 2018 https://github.com/mandreyel/
*/

#ifndef WXLIB_MIO_HPP
#define WXLIB_MIO_HPP

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#else
#define INVALID_HANDLE_VALUE -1
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>

namespace mio {

/**
   This is used by `basic_mmap` to determine whether to create a read-only
   or a read-write memory mapping.
 */
enum class access_mode
{
  read,
  write
};

/**
   Determines the operating system's page allocation granularity.

   On the first call to this function, it invokes relevant OS specific APIs
   to determine the page size, then caches the value and returns it. On any
   subsequent calls, this function just returns the cached value, without
   calling the OS APIs again.

   \returns A size_t.
 */
inline size_t page_size()
{
  static const size_t page_size = [] {
#ifdef _WIN32
    SYSTEM_INFO SystemInfo;
    GetSystemInfo(&SystemInfo);
    return SystemInfo.dwAllocationGranularity;
#else
    return sysconf(_SC_PAGE_SIZE);
#endif
  }();

  return page_size;
}

/**
   Aligns `offset` to operating system page size such that it subtracts the
   difference until the nearest page boundary before `offset`, or does
   nothing if `offset` is already page aligned.

   \param   offset  The offset.

   \returns A size_t.
 */
inline size_t make_offset_page_aligned(size_t offset)
{
  const size_t page_size_ = page_size();
  // Use integer division to round down to the nearest page alignment.
  return offset / page_size_ * page_size_;
}

/**
   This value can serve as the `length` parameter to the constructor of
   `map`, indicating a memory mapping of the entire file is created.
 */
enum
{
  map_entire_file = 0
};

#ifdef _WIN32
using file_handle_type = HANDLE;
#else
using file_handle_type = int;
#endif

/**
   Represents an invalid file handle type. It can be used to determine
   whether `basic_mmap::file_handle` is valid.
 */
const static file_handle_type invalid_handle = INVALID_HANDLE_VALUE;

namespace detail {

#ifdef _WIN32

/*!
 * Standard string type.
 * @tparam C
 */
template<typename C>
concept StringType = (std::is_same_v<typename C::value_type, char> || std::is_same_v<typename C::value_type, wchar_t>)
    && requires { std::declval<C>().c_str(); };

template<typename S, StringType C = std::remove_cvref_t<S>> //
struct char_type_helper
{
  using type = typename C::value_type;
};

template<typename S, StringType C = std::remove_cvref_t<S>>
using char_type_helper_t = typename char_type_helper<S>::type;

#else

/*!
 * Standard string type.
 * @tparam C
 */
template<typename C>
concept StringType = (std::is_same_v<typename C::value_type, char>) && requires { std::declval<C>().data(); };

template<typename S, StringType C = std::remove_cvref_t<S>>
struct char_type_helper
{
    using type = typename C::value_type;
};

template<typename S, StringType C = std::remove_cvref_t<S>>
using char_type_helper_t = typename char_type_helper<S>::type;
#endif

template<typename T>
struct char_type
{
  using type = typename char_type_helper<T>::type;
};

template<typename T>
using char_type_t = typename char_type<T>::type;

template<>
struct char_type<char *>
{
  using type = char;
};

template<>
struct char_type<const char *>
{
  using type = char;
};

template<size_t N>
struct char_type<char[N]>
{
  using type = char;
};

template<size_t N>
struct char_type<const char[N]>
{
  using type = char;
};

#ifdef _WIN32
template<>
struct char_type<wchar_t *>
{
  using type = wchar_t;
};

template<>
struct char_type<const wchar_t *>
{
  using type = wchar_t;
};

template<size_t N>
struct char_type<wchar_t[N]>
{
  using type = wchar_t;
};

template<size_t N>
struct char_type<const wchar_t[N]>
{
  using type = wchar_t;
};
#endif

template<typename CharT, typename S>
struct is_c_str_helper
{
  static constexpr bool value = std::is_same_v<
      CharT *,
      std::add_pointer_t<std::remove_cv_t<std::remove_pointer_t<std::remove_cvref_t<S>>>>
  >;
};

template<typename CharT, typename S>
inline constexpr bool is_c_str_helper_v = is_c_str_helper<CharT, S>::value;

template<typename S>
struct is_c_str
{
  static constexpr bool value = is_c_str_helper_v<char, S>;
};

template<typename S>
inline constexpr bool is_c_str_v = is_c_str<S>::value;

#ifdef _WIN32
template<typename S>
struct is_c_wstr
{
  static constexpr bool value = is_c_str_helper<wchar_t, S>::value;
};

template<typename S>
inline constexpr bool is_c_wstr_v = is_c_wstr<S>::value;
#endif // _WIN32

template<typename S>
struct is_c_str_or_c_wstr
{
  static constexpr bool value = is_c_str_v<S>
#ifdef _WIN32
      || is_c_wstr_v<S>
#endif
  ;
};

template<typename S>
inline constexpr bool is_c_str_or_c_wstr_v = is_c_str_or_c_wstr<S>::value;

/*!
 * C-style null terminated string.
 * @tparam S
 */
template<typename S>
concept CStrType = is_c_str_or_c_wstr_v<S>;

template<StringType T>
const char_type_t<T> *c_str(const T &path)
{
  // We want to exclude std::array<char, N>; its data() is not null-terminated.
  return path.c_str();
}

/*!
 * A type that is not a C-style string, and is emptiable.
 * @tparam S 
 */
template<typename S>
concept Emptiable = requires { std::declval<S>().empty(); } && (!is_c_str_or_c_wstr_v<S>);

template<Emptiable T>
bool empty(const T &path)
{
  return path.empty();
}

template<CStrType T>
const char_type_t<T> *c_str(T path)
{
  return path;
}

template<CStrType T>
bool empty(T path)
{
  return !path || (*path == 0);
}

#ifdef _WIN32
namespace win {

inline constexpr DWORD int64_high(int64_t n) noexcept
{
  return n >> 32;
}

inline constexpr DWORD int64_low(int64_t n) noexcept
{
  return n & 0xffffffff;
}

template<typename StrT>
requires(std::is_same_v<char_type_t<StrT>, char>)
file_handle_type open_file_helper(const StrT &path, const access_mode mode)
{
  return ::CreateFileA(c_str(path),
                       mode == access_mode::read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       0);
}

template<typename StrT>
requires(std::is_same_v<char_type_t<StrT>, wchar_t>)
file_handle_type open_file_helper(const StrT &path, const access_mode mode)
{
  return ::CreateFileW(c_str(path),
                       mode == access_mode::read ? GENERIC_READ : GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       0,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       0);
}

} // win
#endif

/**
   Returns the last platform specific system error (errno on POSIX and
   GetLastError on Win) as a `std::error_code`.

   \returns A std::error_code.
 */
inline std::error_code last_error() noexcept
{
  std::error_code error;
#ifdef _WIN32
  error.assign(static_cast<int>(GetLastError()), std::system_category());
#else
  error.assign(errno, std::system_category());
#endif
  return error;
}

template<typename StrT>
file_handle_type open_file(const StrT &path, const access_mode mode, std::error_code &error)
{
  error.clear();
  if (detail::empty(path)) {
    error = std::make_error_code(std::errc::invalid_argument);
    return invalid_handle;
  }

#ifdef _WIN32
  const auto handle = win::open_file_helper(path, mode);
#else // POSIX
  const auto handle = ::open(c_str(path), mode == access_mode::read ? O_RDONLY : O_RDWR);
#endif

  if (handle == invalid_handle) {
    error = detail::last_error();
  }

  return handle;
}

inline uint64_t query_file_size(file_handle_type handle, std::error_code &error) noexcept
{
  error.clear();

#ifdef _WIN32
  LARGE_INTEGER file_size; // a 64 bit signed integer.
  if (::GetFileSizeEx(handle, &file_size) == 0) {
    error = detail::last_error();
    return 0;
  }
  return static_cast<uint64_t>(file_size.QuadPart);
#else // POSIX
  struct stat sbuf;
    if (::fstat(handle, &sbuf) == -1) {
        error = detail::last_error();
        return 0;
    }
    return sbuf.st_size;
#endif
}

struct mmap_context
{
  char *data;
  uint64_t length;
  uint64_t mapped_length; // may be larger than length due to alignment.
#ifdef _WIN32
  file_handle_type file_mapping_handle;
#endif
};

inline mmap_context memory_map(const file_handle_type file_handle,
                               const size_t offset,
                               const size_t length,
                               const access_mode mode,
                               std::error_code &error)
{
  mmap_context result = {
      nullptr,
      0,
      0
#ifdef _WIN32
      , nullptr
#endif
  };

  const size_t aligned_offset = make_offset_page_aligned(offset);
  const uint64_t length_to_map = offset - aligned_offset + length;

  if (length_to_map > std::numeric_limits<size_t>::max() - 1) {
    error = std::make_error_code(std::errc::invalid_argument);
    return result;
  }

#ifdef _WIN32
  const auto max_file_size = static_cast<int64_t>(offset + length);

  const file_handle_type file_mapping_handle = ::CreateFileMapping(
      file_handle,
      nullptr,
      mode == access_mode::read ? PAGE_READONLY : PAGE_READWRITE,
      win::int64_high(max_file_size),
      win::int64_low(max_file_size),
      nullptr);

  if (file_mapping_handle == nullptr) {
    error = detail::last_error();
    return result;
  }

  LPVOID mapping_start = ::MapViewOfFile(
      file_mapping_handle,
      mode == access_mode::read ? FILE_MAP_READ : FILE_MAP_WRITE,
      win::int64_high(static_cast<int64_t>(aligned_offset)),
      win::int64_low(static_cast<int64_t>(aligned_offset)),
      static_cast<ULONG_PTR>(length_to_map));

  if (mapping_start == nullptr) {
    error = detail::last_error();
    return result;
  }
#else // POSIX
  char *mapping_start = static_cast<char *>(::mmap(
        0, // Don't give hint as to where to map.
        length_to_map,
        mode == access_mode::read ? PROT_READ : PROT_WRITE,
        MAP_SHARED,
        file_handle,
        aligned_offset));

    if (mapping_start == MAP_FAILED) {
        error = detail::last_error();
        return result;
    }
#endif

  result.data = [&](std::span<char> data_) -> char * {
    return &data_[offset - aligned_offset];
  }({static_cast<char *>(mapping_start),
     static_cast<size_t>(static_cast<ptrdiff_t>(offset - aligned_offset + 1))});

  result.length = length;
  result.mapped_length = length_to_map;
#ifdef _WIN32
  result.file_mapping_handle = file_mapping_handle;
#endif
  return result;
}

} // namespace detail

#pragma region - template<access_mode AccessMode, typename ByteT> basic_map

template<access_mode AccessMode, typename ByteT>
struct basic_mmap
{
  using difference_type = std::ptrdiff_t;
  using handle_type = file_handle_type;
  using iterator_category = std::random_access_iterator_tag;
  using size_type = size_t;
  using value_type = ByteT;
  // pointer type
  using pointer = value_type *;
  using const_pointer = const value_type *;
  // iterator type
  using iterator = pointer;
  using const_iterator = const_pointer;
  // reference type
  using reference = value_type &;
  using const_reference = const value_type &;
  // reverse_iterator type
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  static_assert(
      sizeof(ByteT) == sizeof(char), "ByteT must be same size as char.");
private:
  // Points to the first requested byte, not actual start of the mapping.
  pointer data_ = nullptr;
  // Length in bytes requested by user (which may not be full mapping length).
  size_type length_ = 0;
  // Length in bytes of full mapping.
  size_type mapped_length_ = 0;
  // Letting user map a file using both an existing file handle and a path
  // introduces some complexity (see `is_handle_internal_`).
  // On POSIX, we only need a file handle to create a mapping, while on
  // Windows systems the file handle is necessary to retrieve a file mapping
  // handle, but any subsequent operations on the mapped region must be done
  // through the latter.
  handle_type file_handle_ = INVALID_HANDLE_VALUE;
#ifdef _WIN32
  handle_type file_mapping_handle_ = INVALID_HANDLE_VALUE;
#endif
  // Letting user map a file using both an existing file handle and a path
  // introduces some complexity in that we must not close the file handle if
  // user provided it, but we must close it if we obtained it using the
  // provided path. For this reason, this flag is used to determine when to
  // close `file_handle_`.
  bool is_handle_internal_{};

public:

  /**
     Default constructed mmap object is in non-mapped state. Any operation
     attempting to access nonexistent underlying data will result in undefined
     behavior/segmentation faults.
   */
  basic_mmap() = default;

  /**
     The same as invoking `map` method, except any error is wrapped in
     `std::system_error` and is thrown.

     \exception   std::system_error   Raised when a system error occurs.

     \tparam  StrT    string type, char or wchar_t.
     \param   path    Full pathname of the file.
     \param   offset  (Optional) The offset.
     \param   length  (Optional) The length.
   */
  template<typename StrT>
  [[maybe_unused]] explicit basic_mmap(const StrT &path, const size_type offset = 0, const size_type length = map_entire_file)
  {
    std::error_code error;
    map(path, offset, length, error);
    if (error) throw std::system_error(error);
  }

  /**
     The same as invoking `map` method, except any error is wrapped in
     `std::system_error` and is thrown.

     \exception   std::system_error   Raised when a system error occurs.

     \param   handle  The handle.
     \param   offset  (Optional) The offset.
     \param   length  (Optional) The length.
   */
  [[maybe_unused]] explicit basic_mmap(const handle_type handle,
                                       const size_type offset = 0,
                                       const size_type length = map_entire_file)
  {
    std::error_code error;
    map(handle, offset, length, error);
    if (error) throw std::system_error(error);

  }

  basic_mmap(const basic_mmap &) = delete;
  basic_mmap &operator=(const basic_mmap &) = delete;

  /**
     `basic_mmap` has single-ownership semantics. Transferring ownership may
     only be accomplished by moving the object.

     \param [in,out]  other   R-value to move from.
   */
  basic_mmap(basic_mmap &&other) noexcept:
      data_{std::move(other.data_)},
      length_{std::move(other.length_)},
      mapped_length_{std::move(other.mapped_length_)},
      file_handle_{std::move(other.file_handle_)}
#ifdef _WIN32
      , file_mapping_handle_{std::move(other.file_mapping_handle_)}
#endif
      , is_handle_internal_{std::move(other.is_handle_internal_)}
  {
    other.data_ = nullptr;
    other.length_ = other.mapped_length_ = 0;
    other.file_handle_ = invalid_handle;
#ifdef _WIN32
    other.file_mapping_handle_ = invalid_handle;
#endif
  }

  /**
     `basic_mmap` has single-ownership semantics. Transferring ownership may
     only be accomplished by moving the object.

     \param [in,out]  other   R-value to move from.

     \returns A shallow copy of this object.
   */
  basic_mmap &operator=(basic_mmap &&other) noexcept
  {
    if (this != &other) {
      unmap(); // Remove existing.
      data_ = std::move(other.data_);
      length_ = std::move(other.length_);
      mapped_length_ = std::move(other.mapped_length_);
      file_handle_ = std::move(other.file_handle_);
#ifdef _WIN32
      file_mapping_handle_ = std::move(other.file_mapping_handle_);
#endif
      is_handle_internal_ = std::move(other.is_handle_internal_);
      other.data_ = nullptr;
      other.length_ = other.mapped_length_ = 0;
      other.file_handle_ = invalid_handle;
#ifdef _WIN32
      other.file_mapping_handle_ = invalid_handle;
#endif
      other.is_handle_internal_ = false;
    }
    return *this;
  }

  /**
     If this is a read-write mapping, the destructor invokes sync. Regardless
     of the access mode, unmap is invoked as a final step.
   */
  ~basic_mmap()
  {
    conditional_sync();
    unmap();
  }

  /**
     On UNIX systems 'file_handle' and 'mapping_handle' are the same. On
     Windows, however, a mapped region of a file gets its own handle, which is
     returned by 'mapping_handle'.

     \returns A handle_type.
   */
  [[nodiscard]] handle_type file_handle() const noexcept
  {
    return file_handle_;
  }

  /**
     On UNIX systems 'file_handle' and 'mapping_handle' are the same. On
     Windows, however, a mapped region of a file gets its own handle, which is
     returned by 'mapping_handle'.

     \returns A handle_type.
   */
  [[nodiscard]] handle_type mapping_handle() const noexcept
  {
#ifdef _WIN32
    return file_mapping_handle_;
#else
    return file_handle_;
#endif
  }

  /**
     Returns whether a valid memory mapping has been created.

     \returns True if open, false if not.
   */
  [[nodiscard]] bool is_open() const noexcept
  {
    return file_handle_ != invalid_handle;
  }

  /**
     Returns true if no mapping was established, that is, conceptually the
     same as though the length that was mapped was 0. This function is
     provided so that this class has Container semantics.

     \returns True if it is empty, false otherwise.
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return length() == 0;
  }

  /**
     Returns true if a mapping was established. On UNIX, it is the same as
     is_open().

     \returns True if mapped, false if not.
   */
  [[nodiscard]] bool is_mapped() const noexcept
  {
#ifdef _WIN32
    return file_mapping_handle_ != invalid_handle;
#else // POSIX
    return is_open();
#endif
  }

  /**
     `size` and `length` both return the logical length, i.e. the number of
     bytes user requested to be mapped, while `mapped_length` returns the
     actual number of bytes that were mapped which is a multiple of the
     underlying operating system's page allocation granularity.

     \returns A size_type.
   */
  [[nodiscard]] size_type size() const noexcept
  {
    return length();
  }

  [[nodiscard]] size_type length() const noexcept
  {
    return length_;
  }

  [[nodiscard]] size_type mapped_length() const noexcept
  {
    return mapped_length_;
  }

  /**
     Returns the offset relative to the start of the mapping.
   */
  [[nodiscard]] size_type mapping_offset() const noexcept
  {
    return mapped_length_ - length_;
  }

  /**
     Returns a pointer to the first requested byte, or `nullptr` if no
     memory mapping exists.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  pointer data() noexcept
  {
    return data_;
  }

  [[nodiscard]] const_pointer data() const noexcept
  {
    return data_;
  }

  /**
     Returns an iterator to the first requested byte, if a valid memory
     mapping exists, otherwise this function call is undefined behavior.
    */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  iterator begin() noexcept
  {
    return data();
  }

  [[nodiscard]] const_iterator begin() const noexcept
  {
    return data();
  }

  [[nodiscard]] const_iterator cbegin() const noexcept
  {
    return data();
  }

  /**
     Returns an iterator one past the last requested byte, if a valid memory
     mapping exists, otherwise this function call is undefined behavior.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  iterator end() noexcept
  {
    return data() + length();
  }

  [[maybe_unused]][[nodiscard]] const_iterator end() const noexcept
  {
    return data() + length();
  }

  [[maybe_unused]] [[nodiscard]] const_iterator cend() const noexcept
  {
    return data() + length();
  }

  /**
     Returns a reverse iterator to the last memory mapped byte, if a valid
     memory mapping exists, otherwise this function call is undefined behavior.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  reverse_iterator rbegin() noexcept
  {
    return reverse_iterator(end());
  }

  [[nodiscard]] [[maybe_unused]] const_reverse_iterator rbegin() const noexcept
  {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] const_reverse_iterator crbegin() const noexcept
  {
    return const_reverse_iterator(end());
  }

  /**
     Returns a reverse iterator past the first mapped byte, if a valid memory
     mapping exists, otherwise this function call is undefined behavior.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  reverse_iterator rend() noexcept
  {
    return reverse_iterator(begin());
  }

  [[maybe_unused]] [[nodiscard]] const_reverse_iterator rend() const noexcept
  {
    return const_reverse_iterator(begin());
  }

  [[nodiscard]] const_reverse_iterator crend() const noexcept
  {
    return const_reverse_iterator(begin());
  }

  /**
     Returns a reference to the `i`th byte from the first requested byte (as
     returned by `data`). If no valid memory mapping has been created prior
     to this call, undefined behavior ensues.
   */
  reference operator[](const size_type i) noexcept
  {
    return data_[i];
  }

  const_reference operator[](const size_type i) const noexcept
  {
    return data_[i];
  }

  /**
     Establishes a memory mapping with AccessMode. On unsuccessful mapping,
     the reason is reported via `error` and the object remains in a state as
     if this function hadn't been called.

     `path`, which must refers to an existing file, is used to retrieve a
     file handle (which is closed when the object destructs or `unmap` is
     called), which is then used to memory map the requested region. Upon
     failure, `error` is set to indicate the reason and the object remains in
     an unmapped state.

     `offset` is the number of bytes, relative to the start of the file, where
     the mapping should begin. When specifying it, there is no need to worry
     about providing a value that is aligned with the operating system's page
     allocation granularity. This is adjusted by the implementation such that
     the first requested byte (as returned by `data` or `begin`), so long as
     `offset` is valid, will be at `offset` from the start of the file.

     `length` is the number of bytes to map. It may be `map_entire_file`, in
     which case a mapping of the entire file is created.
   */
  template<typename StrT>
  void map(const StrT &path, const size_type offset, const size_type length, std::error_code &error)
  {
    error.clear();

    if (detail::empty(path)) {
      error = std::make_error_code(std::errc::invalid_argument);
      return;
    }

    const auto handle = detail::open_file(path, AccessMode, error);
    if (error) return;

    map(handle, offset, length, error);

    // MUST sets this to true.
    if (!error) is_handle_internal_ = true;
  }

  /**
      Map the entire file.
   */
  template<typename StrT>
  void map(const StrT &path, std::error_code &error)
  {
    map(path, 0, map_entire_file, error);
  }

  /**
     Establish memory mapping using file handle.
   */
  void map(const handle_type handle, const size_type offset, const size_type length, std::error_code &error)
  {
    error.clear();

    if (handle == invalid_handle) {
      error = std::make_error_code(std::errc::bad_file_descriptor);
      return;
    }

    const uint64_t file_size = detail::query_file_size(handle, error);
    if (error) return;

    const uint64_t requested_size = offset + (length == map_entire_file ? (file_size - offset) : length);

    // Guarantees requested size is smaller than address space limit.
    if (requested_size > file_size || requested_size > std::numeric_limits<size_t>::max() - 1) {
      error = std::make_error_code(std::errc::invalid_argument);
      return;
    }

    const auto ctx = detail::memory_map(
        handle,
        offset,
        static_cast<size_type>(requested_size - offset),
        AccessMode,
        error);

    if (!error) {
      // unmap previous mapping that may have existed. This guarantees that,
      // should the new mapping fail, the `map` function leaves this instance
      // in a state as though the function had never been invoked.
      unmap();
      file_handle_ = handle;
      is_handle_internal_ = false;

      if constexpr (std::is_same_v<pointer, decltype(ctx.data)>)
        data_ = ctx.data;
      else
        data_ = static_cast<pointer>(ctx.data);

      length_ = static_cast<size_type>(ctx.length);
      mapped_length_ = static_cast<size_type>(ctx.mapped_length);
#ifdef _WIN32
      file_mapping_handle_ = ctx.file_mapping_handle;
#endif
    }
  }

  /**
     Establish memory mapping using file handle for the entire file.
   */
  void map(const handle_type handle, std::error_code &error)
  {
    map(handle, 0, map_entire_file, error);
  }

  /**
     If a valid memory mapping has been created prior to this call, this call
     instructs the kernel to unmap the memory region and disassociate this
     object from the file.

     The file handle associated with the file that is mapped is only closed if
     the mapping was created using a file path. If, on the other hand, an
     existing file handle was used to create the mapping, the file handle is
     not closed.
   */
  void unmap() noexcept
  {
    if (!is_open()) return;

    // TODO do we care about errors here?
#ifdef _WIN32
    if (is_mapped()) {
      ::UnmapViewOfFile(get_mapping_start());
      ::CloseHandle(file_mapping_handle_);
    }
#else // POSIX
    if (data_) {
        ::munmap(const_cast<pointer>(get_mapping_start()), mapped_length_);
    }
#endif

    // If `file_handle_` was obtained by our opening it (when map is called with
    // a path, rather than an existing file handle), we need to close it,
    // otherwise it must not be closed as it may still be used outside this
    // instance.
    if (is_handle_internal_) {
#ifdef _WIN32
      ::CloseHandle(file_handle_);
#else // POSIX
      ::close(file_handle_);
#endif
    }

    // Reset fields to their default values.
    data_ = nullptr;
    length_ = mapped_length_ = 0;
    file_handle_ = invalid_handle;
#ifdef _WIN32
    file_mapping_handle_ = invalid_handle;
#endif
  }

  [[maybe_unused]] void swap(basic_mmap &other)
  {
    if (this != &other) {
      std::swap(data_, other.data_);
      std::swap(file_handle_, other.file_handle_);
#ifdef _WIN32
      std::swap(file_mapping_handle_, other.file_mapping_handle_);
#endif
      std::swap(length_, other.length_);
      std::swap(mapped_length_, other.mapped_length_);
      std::swap(is_handle_internal_, other.is_handle_internal_);
    }
  }

  /**
     Flushes the memory mapped page to disk. Errors are reported via `error`.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  void sync(std::error_code &error)
  {
    error.clear();

    if (!is_open()) {
      error = std::make_error_code(std::errc::bad_file_descriptor);
      return;
    }

    if (data()) {
#ifdef _WIN32
      if (::FlushViewOfFile(get_mapping_start(), mapped_length_) == 0 || ::FlushFileBuffers(file_handle_) == 0) {
#else // POSIX
        if (::msync(get_mapping_start(), mapped_length_, MS_SYNC) != 0) {
#endif
        error = detail::last_error();
        return;
      }
    }
#ifdef _WIN32
    if (::FlushFileBuffers(file_handle_) == 0) {
      error = detail::last_error();
    }
#endif
  }

private:
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  pointer get_mapping_start() noexcept
  {
    // Compare the address of the first byte and size of the two mapped region
    return !data() ? nullptr : data() - mapping_offset();
  }

  [[nodiscard]] const_pointer get_mapping_start() const noexcept
  {
    // Compare the address of the first byte and size of the two mapped region
    return !data() ? nullptr : data() - mapping_offset();
  }

  /**
     The destructor syncs changes to disk if `AccessMode` is `write`, but not
     if it's `read`. Because the destructor cannot be a template, we need to
     do SFINAE, so one syncs and the other is a no-op.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  void conditional_sync()
  {
    // Invoked from destructor, so not much we can do about failures here.
    std::error_code ec;
    sync(ec);
  }

  template<access_mode A = AccessMode>
  requires (A == access_mode::read)
  void conditional_sync()
  {
    // no-op.
  }
};
#pragma endregion

#pragma region - operator overloading for basic_map class
template<access_mode AccessMode, typename ByteT>
bool operator==(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return (a.data() == b.data()) && (a.size() == b.size());
}

template<access_mode AccessMode, typename ByteT>
bool operator!=(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return !(a == b);
}

template<access_mode AccessMode, typename ByteT>
bool operator<(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return a.data() == b.data() ? (a.size() < b.size()) : (a.data() < b.data());
}

template<access_mode AccessMode, typename ByteT>
bool operator<=(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return (a <= b);
}

template<access_mode AccessMode, typename ByteT>
bool operator>(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return (a.data() == b.data()) ? (a.size() > b.size()) : (a.data() > b.data());
}

template<access_mode AccessMode, typename ByteT>
bool operator>=(const basic_mmap<AccessMode, ByteT> &a, const basic_mmap<AccessMode, ByteT> &b)
{
  return !(a < b);
}
#pragma endregion

#pragma region - type alias for common use cases of basic_map template

/**
   This is the basis for all read-only mmap objects and should be preferred
   over directly using `basic_mmap`.
 */
template<typename ByteT>
using basic_mmap_source = basic_mmap<access_mode::read, ByteT>;

/**
   This is the basis for all read-write mmap objects and should be preferred
   over directly using `basic_mmap`.
 */
template<typename ByteT>
using basic_mmap_sink = basic_mmap<access_mode::write, ByteT>;

/**
   These aliases cover the most common use cases, both representing a raw
   byte stream (either with a char or an unsigned char/uint8_t).
 */
using mmap_source = basic_mmap_source<char>;
using ummap_source [[maybe_unused]] = basic_mmap_source<unsigned char>;

using mmap_sink = basic_mmap_sink<char>;
using ummap_sink [[maybe_unused]] = basic_mmap_sink<unsigned char>;
#pragma endregion

#pragma region - factory methods for basic_mmap

/**
   Convenience factory method that constructs a mapping for any `basic_mmap`
   or `basic_mmap` type.
 */
template<typename MMap, typename MappingToken>
MMap make_mmap(const MappingToken &token, typename MMap::size_type offset,
               typename MMap::size_type length, std::error_code &error)
{
  MMap mmap;
  mmap.map(token, offset, length, error);
  return mmap;
}

/**
   Convenience factory method.

   MappingToken may be a string (`std::string`, `std::string_view`, `const
   char*`, `std::filesystem::path`, `std::vector&lt;char&gt;`, or similar),
   or a `mmap_source::handle_type`.
 */
template<typename MappingToken>
mmap_source make_mmap_source(const MappingToken &token,
                             mmap_source::size_type offset,
                             mmap_source::size_type length,
                             std::error_code &error)
{
  return make_mmap<mmap_source>(token, offset, length, error);
}

template<typename MappingToken>
[[maybe_unused]] mmap_source make_mmap_source(const MappingToken &token, std::error_code &error)
{
  return make_mmap_source(token, 0, map_entire_file, error);
}

/**
   Convenience factory method.

   MappingToken may be a string (`std::string`, `std::string_view`, `const
   char*`, `std::filesystem::path`, `std::vector&lt;char&gt;`, or similar),
   or a `mmap_sink::handle_type`.
 */
template<typename MappingToken>
mmap_sink make_mmap_sink(const MappingToken &token,
                         mmap_sink::size_type offset,
                         mmap_sink::size_type length,
                         std::error_code &error)
{
  return make_mmap<mmap_sink>(token, offset, length, error);
}

template<typename MappingToken>
[[maybe_unused]] mmap_sink make_mmap_sink(const MappingToken &token, std::error_code &error)
{
  return make_mmap_sink(token, 0, map_entire_file, error);
}
#pragma endregion

#pragma region - basic_shared_mmap

/**
   Exposes (nearly) the same interface as `basic_mmap`, but endows it with
   `std::shared_ptr` semantics.
   
   If shared semantics are not required, do not use it.
 */
template<access_mode AccessMode, typename ByteT>
class basic_shared_mmap
{
  using impl_type = basic_mmap<AccessMode, ByteT>;
  std::shared_ptr<impl_type> pimpl_;

public:
  using value_type = typename impl_type::value_type;
  using size_type = typename impl_type::size_type;
  using reference = typename impl_type::reference;
  using const_reference = typename impl_type::const_reference;
  using pointer = typename impl_type::pointer;
  using const_pointer = typename impl_type::const_pointer;
  using difference_type [[maybe_unused]] = typename impl_type::difference_type;
  using iterator = typename impl_type::iterator;
  using const_iterator = typename impl_type::const_iterator;
  using reverse_iterator = typename impl_type::reverse_iterator;
  using const_reverse_iterator = typename impl_type::const_reverse_iterator;
  using iterator_category [[maybe_unused]] = typename impl_type::iterator_category;
  using handle_type = typename impl_type::handle_type;
  using mmap_type = impl_type;

  basic_shared_mmap() = default;
  basic_shared_mmap(const basic_shared_mmap &) = default;
  basic_shared_mmap &operator=(const basic_shared_mmap &) = default;
  basic_shared_mmap(basic_shared_mmap &&) noexcept = default;
  basic_shared_mmap &operator=(basic_shared_mmap &&) noexcept = default;

  /** Takes ownership of an existing mmap object. */
  [[maybe_unused]] explicit basic_shared_mmap(mmap_type &&mmap) : pimpl_{std::make_shared<mmap_type>(std::move(mmap))}
  {
  }

  /** Takes ownership of an existing mmap object. */
  basic_shared_mmap &operator=(mmap_type &&mmap)
  {
    pimpl_ = std::make_shared<mmap_type>(std::move(mmap));
    return *this;
  }

  /** Initializes this object with an already established shared mmap. */
  [[maybe_unused]] explicit basic_shared_mmap(std::shared_ptr<mmap_type> mmap) : pimpl_(std::move(mmap))
  {
  }

  /** Initializes this object with an already established shared mmap. */
  basic_shared_mmap &operator=(std::shared_ptr<mmap_type> mmap)
  {
    pimpl_ = std::move(mmap);
    return *this;
  }

  /**
     The same as invoking the `map` function, except any error that may occur
     while establishing the mapping is wrapped in a `std::system_error` and
     is thrown.
   */
  template<typename StrT>
  [[maybe_unused]] explicit basic_shared_mmap(const StrT &path,
                                              const size_type offset = 0,
                                              const size_type length = map_entire_file)
  {
    std::error_code error;
    map(path, offset, length, error);
    if (error) throw std::system_error(error);
  }

  /**
     The same as invoking the `map` function, except any error that may occur
     while establishing the mapping is wrapped in a `std::system_error` and
     is thrown.
   */
  [[maybe_unused]] explicit basic_shared_mmap(const handle_type handle,
                                              const size_type offset = 0,
                                              const size_type length = map_entire_file)
  {
    std::error_code error;
    map(handle, offset, length, error);
    if (error) throw std::system_error(error);
  }

  /**
     If this is a read-write mapping and the last reference to the mapping,
     the destructor invokes sync. Regardless of the access mode, unmap is
     invoked as a final step.
   */
  ~basic_shared_mmap() = default;

  /** Returns the underlying `std::shared_ptr` instance that holds the mmap.*/
  [[maybe_unused]] std::shared_ptr<mmap_type> get_shared_ptr()
  {
    return pimpl_;
  }

  /**
     On UNIX systems 'file_handle' and 'mapping_handle' are the same. On
     Windows, however, a mapped region of a file gets its own handle, which is
     returned by 'mapping_handle'.
   */
  handle_type file_handle() const noexcept
  {
    return pimpl_ ? pimpl_->file_handle() : invalid_handle;
  }

  [[maybe_unused]] handle_type mapping_handle() const noexcept
  {
    return pimpl_ ? pimpl_->mapping_handle() : invalid_handle;
  }

  /** Returns whether a valid memory mapping has been created. */
  [[nodiscard]] bool is_open() const noexcept
  {
    return pimpl_ && pimpl_->is_open();
  }

  /**
     Returns true if no mapping was established, that is, conceptually the
     same as though the length that was mapped was 0. This function is
     provided so that this class has Container semantics.
   */
  [[nodiscard]] bool empty() const noexcept
  {
    return !pimpl_ || pimpl_->empty();
  }

  /**
     `size` and `length` both return the logical length, i.e. the number of
     bytes user requested to be mapped, while `mapped_length` returns the
     actual number of bytes that were mapped which is a multiple of the
     underlying operating system's page allocation granularity.
   */
  size_type size() const noexcept
  {
    return pimpl_ ? pimpl_->length() : 0;
  }

  [[maybe_unused]] size_type length() const noexcept
  {
    return pimpl_ ? pimpl_->length() : 0;
  }

  [[maybe_unused]] size_type mapped_length() const noexcept
  {
    return pimpl_ ? pimpl_->mapped_length() : 0;
  }

  /**
     Returns a pointer to the first requested byte, or `nullptr` if no
     memory mapping exists.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  pointer data() noexcept
  {
    return pimpl_->data();
  }

  const_pointer data() const noexcept
  {
    return pimpl_ ? pimpl_->data() : nullptr;
  }

  /**
     Returns an iterator to the first requested byte, if a valid memory
     mapping exists, otherwise this function call is undefined behavior.
   */
  iterator begin() noexcept
  {
    return pimpl_->begin();
  }

  const_iterator begin() const noexcept
  {
    return pimpl_->begin();
  }

  [[maybe_unused]] const_iterator cbegin() const noexcept
  {
    return pimpl_->cbegin();
  }

  /**
     Returns an iterator one past the last requested byte, if a valid memory
     mapping exists, otherwise this function call is undefined behavior.
   */
  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  iterator end() noexcept
  {
    return pimpl_->end();
  }

  const_iterator end() const noexcept
  {
    return pimpl_->end();
  }

  [[maybe_unused]] const_iterator cend() const noexcept
  {
    return pimpl_->cend();
  }

  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  [[maybe_unused]] reverse_iterator rbegin() noexcept
  {
    return pimpl_->rbegin();
  }

  [[maybe_unused]] const_reverse_iterator rbegin() const noexcept
  {
    return pimpl_->rbegin();
  }

  [[maybe_unused]] const_reverse_iterator crbegin() const noexcept
  {
    return pimpl_->crbegin();
  }

  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  [[maybe_unused]] reverse_iterator rend() noexcept
  {
    return pimpl_->rend();
  }

  [[maybe_unused]] const_reverse_iterator rend() const noexcept
  {
    return pimpl_->rend();
  }

  [[maybe_unused]] const_reverse_iterator crend() const noexcept
  {
    return pimpl_->crend();
  }

  /**
     Returns a reference to the `i`th byte from the first requested byte (as
     returned by `data`). If this is invoked when no valid memory mapping has
     been created prior to this call, undefined behavior ensues.
   */
  reference operator[](const size_type i) noexcept
  {
    return (*pimpl_)[i];
  }

  const_reference operator[](const size_type i) const noexcept
  {
    return (*pimpl_)[i];
  }

  template<typename StrT>
  void map(const StrT &path, const size_type offset, const size_type length, std::error_code &error)
  {
    map_impl(path, offset, length, error);
  }

  template<typename StrT>
  void map(const StrT &path, std::error_code &error)
  {
    map_impl(path, 0, map_entire_file, error);
  }

  void map(const handle_type handle, const size_type offset, const size_type length, std::error_code &error)
  {
    map_impl(handle, offset, length, error);
  }

  void map(const handle_type handle, std::error_code &error)
  {
    map_impl(handle, 0, map_entire_file, error);
  }

  [[maybe_unused]] void unmap()
  {
    if (pimpl_) pimpl_->unmap();
  }

  [[maybe_unused]] void swap(basic_shared_mmap &other)
  {
    pimpl_.swap(other.pimpl_);
  }

  template<access_mode A = AccessMode>
  requires (A == access_mode::write)
  [[maybe_unused]] void sync(std::error_code &error)
  {
    if (pimpl_) pimpl_->sync(error);
  }

  /** All operators compare the underlying `basic_mmap`'s addresses. */
  friend bool operator==(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return a.pimpl_ == b.pimpl_;
  }

  friend bool operator!=(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return !(a == b);
  }

  friend bool operator<(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return a.pimpl_ < b.pimpl_;
  }

  friend bool operator<=(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return a.pimpl_ <= b.pimpl_;
  }

  friend bool operator>(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return a.pimpl_ > b.pimpl_;
  }

  friend bool operator>=(const basic_shared_mmap &a, const basic_shared_mmap &b)
  {
    return a.pimpl_ >= b.pimpl_;
  }

private:
  template<typename MappingToken>
  void map_impl(const MappingToken &token, const size_type offset, const size_type length, std::error_code &error)
  {
    if (!pimpl_) {
      mmap_type mmap = make_mmap<mmap_type>(token, offset, length, error);
      if (error) return;
      pimpl_ = std::make_shared<mmap_type>(std::move(mmap));
    } else {
      pimpl_->map(token, offset, length, error);
    }
  }
};

/**
   This is the basis for all read-only mmap objects and should be preferred
   over directly using basic_shared_mmap.
 */
template<typename ByteT>
using basic_shared_mmap_source = basic_shared_mmap<access_mode::read, ByteT>;

/**
   This is the basis for all read-write mmap objects and should be preferred
   over directly using basic_shared_mmap.
 */
template<typename ByteT>
using basic_shared_mmap_sink = basic_shared_mmap<access_mode::write, ByteT>;

/**
   These aliases cover the most common use cases, both representing a raw
   byte stream (either with a char or an unsigned char/uint8_t).
 */
using shared_mmap_source [[maybe_unused]] = basic_shared_mmap_source<char>;
using shared_ummap_source [[maybe_unused]] = basic_shared_mmap_source<unsigned char>;

using shared_mmap_sink [[maybe_unused]] = basic_shared_mmap_sink<char>;
using shared_ummap_sink [[maybe_unused]] = basic_shared_mmap_sink<unsigned char>;
#pragma endregion

enum class LoadingMode
{
  Asynchronous, Synchronous
};

} // namespace mio

#endif