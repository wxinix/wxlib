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

    Additional thread ID provided for distinguishing the thread-context.
    Use the ID to facilitate thread-safe data handling.

    Should return status code 0 for success, and non-zero for error.
    When a non-zero error code is returned, the async loading process will be
    terminated immediately with no more lines being loaded thereafter.

    Exceptions, if any, must not escape the callback. They must be handled
    inside the call back, then translated to user-defined error code before returning.
  */
  using AsyncGetlineCallback = std::function<int(int, const std::string_view)>;

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
  [[maybe_unused]] explicit StringReader(const std::string &a_file) : mmap_{a_file}
  {
    begin_ = mmap_.begin();
  }

  explicit StringReader(const std::string &&a_file) : mmap_{a_file}
  {
    begin_ = mmap_.begin();
  }

  StringReader() = delete;
  StringReader(const StringReader &) = delete;
  StringReader(StringReader &&) = delete;
  StringReader &operator=(StringReader &) = delete;
  StringReader &operator=(StringReader &&) = delete;

  ~StringReader()
  {
    mmap_.unmap();
  }

  /**
     Checks whether the reader has reached end of file.

     \returns True if end of line, false otherwise.
   */
  template<typename = void>
  requires (L == LoadingMode::Synchronous)
  [[nodiscard]] bool eof() const noexcept
  {
    return (begin_ == nullptr);
  }

  /**
   Checks whether the reader has successfully mapped the underlying file.
   Only on mapped file can getline be called.

   \returns True if mapped, false otherwise.
 */
  [[nodiscard]] inline bool is_mapped() const noexcept
  {
    return mmap_.is_mapped();
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
    const char *b = begin_;
    const char *find_pos = fast_find<'\n'>(b, mmap_.end());

    // find_pos == mmap_.end() happens only once at end of file. The majority of the
    // processing will be for find_pos != mmap_.end(). Give this hint to the compiler
    // for better branch prediction.
    if (semi_branch_expect((find_pos != mmap_.end()), true))
      begin_ = std::next(find_pos);
    else
      begin_ = (b = nullptr), (find_pos = nullptr); // Set BOTH b AND find_pos nullptr if end of file.

    return {b, static_cast<size_t>(find_pos - b)};
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
    auto line_count = size_t{0};

    // Fall back to synchronous line by line processing.
    while (!this->eof()) {
      // If a non-zero status code is returned, break immediately.
      if (semi_branch_expect((a_callback(this->getline()) == 0), true))
        line_count++;
      else
        break;
    }
    return line_count;
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
    auto futures = std::vector<std::future<size_t>>{};
    for (int i = 0; auto &p : make_partitions(NumThreads))
      futures.emplace_back(std::async(async_getline_impl, i++, p.first, p.second, a_callback));

    // Collect the total number of lines read.
    return std::accumulate(futures.begin(), futures.end(), size_t(0), [](size_t b, auto &&a) { return (a.get() + b); });
  }

private:
  /**
   * A thread worker function for read lines. This is an internal function to be called by getline_async.
   * @param a_thread_id - The thread ID.
   * @param a_begin - The first_iter of the range
   * @param a_end - The last_iter of the range
   * @param a_callback - A getline event callback.
   * @return Total number of lines processed.
   */
  static size_t async_getline_impl(uint8_t a_thread_id,
                                   const char *a_begin,
                                   const char *a_end,
                                   const AsyncGetlineCallback &a_callback) noexcept
  {
    const char *b = a_begin;
    const char *find_pos = fast_find<'\n'>(b, a_end);
    auto counter = size_t{0};

    while (find_pos != a_end) {
      // If a non-zero status code is returned, break immediately.
      if (semi_branch_expect(a_callback(a_thread_id, {b, static_cast<size_t>(find_pos - b)}) == 0, true))
        counter++;
      else
        break;

      b = std::next(find_pos);
      find_pos = fast_find<'\n'>(b, a_end);
    }

    return counter;
  }

  /*!
   * Makes the specified number of partitions.
   * @param a_count The number of partitions to make.
   * @return
   */
  auto make_partitions(const uint8_t a_count) noexcept
  {
    using Partition = std::pair<const char *, const char *>;

    auto result = std::vector<Partition>{};
    const auto part_size = mmap_.size() / a_count;

    const char *b = begin_;
    const char *e = find_end<'\n'>(b, part_size);

    for (uint8_t i = 0; i < a_count; i++) {
      result.emplace_back<Partition>({b, e});

      if (i < a_count - 1) {
        b = e;
        e = (i == a_count - 2) ? mmap_.end() : find_end<'\n'>(b, part_size);
      }
    }

    return result;
  }

private:
  mmap_source mmap_;
  const char *begin_;
};

using StringReaderAsync = StringReader<LoadingMode::Asynchronous>;

}
#endif