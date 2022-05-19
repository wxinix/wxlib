/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

#ifndef WXLIB_MIO_STRING_READER_HPP
#define WXLIB_MIO_STRING_READER_HPP

#include <mio/mio.hpp>
#include <mio/fastfind.hpp>

#include <array>
#include <functional>
#include <future>
#include <iterator>
#include <numeric>
#include <vector>

#ifdef __GNUC__
#define semi_branch_expect(x, y) __builtin_expect(x, y)
#else
#define semi_branch_expect(x, y) x
#endif

namespace mio {

/**
   A fast line reader based on memory mapped file. Supports two loading modes:
   synchronous loading and asynchronous loading.

   Synchronous Loading - the entire file is first mapped into memory, then lines
   of the file are read sequentially, one by one.

   Asynchronous Loading - the file is first mapped into memory, then the mapped
   memory is divided into smaller independent partitions.  Each partition is
   handled in parallel by different threads. Whenever a new line is read, an event
   is fired in its respective thread context for the user specified callback to
   process the line.

   Asynchronous Loading would generally provide the best loading speed. But it
   requires carefully-crafted user code for thread-safe (lock-protected,
   or lock-free) data handling.

   @code
     // Sample for synchronous loading.
     std::filesystem::path file_path = std::filesystem::current_path()/"test.txt";
     assert(std::filesystem::exists(file_path));
     mio::StringReader reader(file_path.string());

     if (reader.is_mapped()) {
       while (!reader.eof()) {
         auto line = reader.getline();
         // ... do something about the line just read.
       }
     }
   @endcode

   @code
     // Sample for asynchronous loading.
     std::filesystem::path file_path = std::filesystem::current_path()/"test.txt";
     assert(std::filesystem::exists(file_path));
     mio::StringReader reader(file_path.string());

     // Defines the event handler to be fired when a new line has been read. The worker ID
     // can be used to distinguish different thread context where the event is fired.
     auto on_getline = [](int a_worker_id, const std::string_view a_line) {
            // ... do something about the line
            return 0; // 0 for success,  non-zero for errors. Must not throw exception.
        };

     if (reader.is_mapped()) {
       // returns total number of lines read
       auto total_lines = reader.getline_async<4>(on_getline); .
     }
   @endcode

 */
template<LoadingMode L = LoadingMode::Synchronous>
class StringReader
{
public:
  /**
    Event that fires asynchronously when a new line has been read.

    Additional thread worker ID provided for distinguishing the thread-context.
    Use the ID to facilitate thread-safe data handling.

    Should return status code 0 for success, and non-zero for error.
    When a non-zero error code is returned, the async loading process will be
    terminated immediately with no more lines being loaded thereafter.

    Exceptions, if any, must not escape the callback. They must be handled
    inside the call back, then translated to user-defined error code before returning.
  */
  using AsyncGetlineCallback = std::function<int(int a_tid, const std::string_view)>;

  /**
    Event that fires synchronously when a new line has been read.

    Should return status code 0 for success, and non-zero for error.
    When a non-zero error code is returned, the loading process will be
    terminated immediately with no more lines being loaded thereafter.

    Exceptions, if any, must not escape the callback. They must be handled
    inside the call back, then translated to user-defined error code before returning.
  */
  using SyncGetlineCallback = std::function<int(const std::string_view)>;

  /**
     Constructs a reader to read from a disk file line by line. If the
     specified file does not exist, std::system_error will be thrown with
     error code describing the nature of the error.

     Precondition - the file to load must exist.

     \param   a_file  The file to read. It must exist.
   */
  [[maybe_unused]] explicit StringReader(const std::string &a_file) : m_mmap{a_file}
  {
    m_begin = m_mmap.begin();
  }

  explicit StringReader(const std::string &&a_file) : m_mmap{a_file}
  {
    m_begin = m_mmap.begin();
  }

  StringReader() = delete;
  StringReader(const StringReader &) = delete;
  StringReader(StringReader &&) = delete;
  StringReader &operator=(StringReader &) = delete;
  StringReader &operator=(StringReader &&) = delete;

  ~StringReader()
  {
    m_mmap.unmap();
  }

  /**
     Checks whether the reader has reached end of file.

     \returns True if end of line, false otherwise.
   */
  template<typename = void>
  requires (L == LoadingMode::Synchronous)
  [[nodiscard]] bool eof() const noexcept
  {
    return (m_begin == nullptr);
  }

  /**
   Checks whether the reader has successfully mapped the underlying file.
   Only on mapped file can getline be called.

   \returns True if mapped, false otherwise.
 */
  [[nodiscard]] inline bool is_mapped() const noexcept
  {
    return m_mmap.is_mapped();
  }

  /**
     Returns a new line that has been read from the file as string view.

     Precondition - StringReader::is_mapped() must be true.

     \returns A std::string_view, {nullptr, 0} will be returned if the reader has reached end of file.
   */
  template<typename = void>
  requires (L == LoadingMode::Synchronous)
  std::string_view getline() noexcept
  {
    const char *l_begin = m_begin;
    const char *l_find = fast_find<'\n'>(l_begin, m_mmap.end());

    // l_find == m_mmap.end() happens only once at end of file. The majority of the
    // processing will be for l_find != m_mmap.end(). Give this hint to the compiler
    // for better branch prediction.
    if (semi_branch_expect((l_find != m_mmap.end()), true))
      m_begin = std::next(l_find);
    else
      m_begin = (l_begin = nullptr), (l_find = nullptr); // Set BOTH l_begin AND l_find nullptr if end of file.

    return {l_begin, static_cast<size_t>(l_find - l_begin)};
  }

  /**
   * Sequentially reads a new line from the file and fires the callback
   * to process the line just read.
   *
   * Precondition - StringReader::is_mapped() must be true.
   *
   * @param a_callback - A callback for processing new lines read.
   * @return Total number of lines processed.
   */
  template<typename = void>
  requires (L == LoadingMode::Synchronous)
  size_t getline(const SyncGetlineCallback &a_callback) noexcept
  {
    size_t l_numlines{0};

    // Fall back to synchronous line by line processing.
    while (!this->eof()) {
      // If a non-zero status code is returned, break immediately.
      if (semi_branch_expect((a_callback(this->getline()) == 0), true))
        l_numlines++;
      else
        break;
    }
    return l_numlines;
  }

  /**
   Reads a new line in the context of a worker thread and fires the callback
   to process the line just read.

   Precondition - StringReader::is_mapped() must be true.

   \param a_callback A callback for processing each of the new line read.
   \tparam NumThreads Number of threads for async processing.

   \returns Total number of lines read.
 */
  template<uint8_t NumThreads>
  requires (NumThreads >= 2) and (NumThreads <= 8) and (L == LoadingMode::Asynchronous)
  size_t async_getline(const AsyncGetlineCallback &a_callback) noexcept
  {
    // Spawn a couple of futures for async processing.
    std::vector<std::future<size_t>> l_futures;

    for (int i = 0; auto &p : mmap_partitions(NumThreads))
      l_futures.emplace_back(std::async(async_getline_impl, i++, p.first, p.second, a_callback));

    // Collect the total number of lines read.
    return std::accumulate(l_futures.begin(), l_futures.end(), size_t(0), [](size_t b, auto &&a) { return (a.get() + b); });
  }

private:
  /**
   * A thread worker function for read lines. This is an internal function to be called by getline_async.
   * @param a_tid - The thread ID.
   * @param a_begin - The first_iter of the range
   * @param a_end - The last_iter of the range
   * @param a_callback - A getline event callback.
   * @return Total number of lines processed.
   */
  static size_t async_getline_impl(uint8_t a_tid,
                                   const char *a_begin,
                                   const char *a_end,
                                   const AsyncGetlineCallback &a_callback) noexcept
  {
    const char *l_begin = a_begin;
    const char *l_find = fast_find<'\n'>(l_begin, a_end);

    size_t l_counter{0};

    while (l_find != a_end) {
      // If a non-zero status code is returned, break immediately.
      if (semi_branch_expect(a_callback(a_tid, {l_begin, static_cast<size_t>(l_find - l_begin)}) == 0, true))
        l_counter++;
      else
        break;

      l_begin = std::next(l_find);
      l_find = fast_find<'\n'>(l_begin, a_end);
    }

    return l_counter;
  }

  auto mmap_partitions(const uint8_t a_numparts) noexcept
  {
    using Partition = std::pair<const char *, const char *>;
    std::vector<Partition> l_partitions;

    const auto l_partition_size = m_mmap.size() / a_numparts;
    const char *l_begin = m_begin;
    const char *l_end = find_end<'\n'>(l_begin, l_partition_size);

    for (uint8_t i = 0; i < a_numparts; i++) {
      l_partitions.emplace_back<Partition>({l_begin, l_end});

      if (i < a_numparts - 1) {
        l_begin = l_end;
        l_end = (i == a_numparts - 2) ? m_mmap.end() : find_end<'\n'>(l_begin, l_partition_size);
      }
    }

    return l_partitions;
  }

private:
  mmap_source m_mmap;
  const char *m_begin;
};

using StringReaderAsync = StringReader<LoadingMode::Asynchronous>;

}
#endif