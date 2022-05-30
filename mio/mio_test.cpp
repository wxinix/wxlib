/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <doctest/doctest.h>

#include <cstring>
#include <string_view>

#include <mio/mio.hpp>
#include <mio/stringreader.hpp>
#include <mio/fastfind.hpp>
#include "mio/csvdoc.hpp"

TEST_CASE("mio")
{
  const auto file_size = 4 * mio::page_size() - 250; // 16134, if page size is 4KiB
  std::string buffer(file_size, 0);

  char v = 33; // Start at first printable ASCII character.
  for (auto &b : buffer) {
    b = v;
    ++v;
    v %= 126; // Limit to last printable ASCII character.
    if (v == 0) v = 33;
  }

  auto path = "test-file";
  std::ofstream file(path);
  file << buffer;
  file.close();

  SUBCASE("test whole file can be mapped") {
    std::error_code error;
    auto offset = 0;

    mio::mmap_source file_view = mio::make_mmap_source(path, offset, mio::map_entire_file, error);
    CHECK(!error);

    CHECK(file_view.is_open());
    CHECK(file_view.size() == buffer.size() - offset);

    for (size_t buf_idx = offset, view_idx = 0; buf_idx < buffer.size() && view_idx < file_view.size(); ++buf_idx, ++view_idx) {
      CHECK(file_view[view_idx] == buffer[buf_idx]);
    }

    mio::shared_mmap_source shared_file_view(std::move(file_view));
    CHECK(!file_view.is_open()); // NOLINT(bugprone-use-after-move)
    CHECK(shared_file_view.is_open());
    CHECK(shared_file_view.size() == buffer.size());
  }

  SUBCASE("test file can be mapped with offset below one page size") {
    std::error_code error;
    auto offset = mio::page_size() - 3;

    mio::mmap_source file_view = mio::make_mmap_source(path, offset, mio::map_entire_file, error);
    CHECK(!error);

    CHECK(file_view.is_open());
    CHECK(file_view.size() == buffer.size() - offset);

    for (size_t buf_idx = offset, view_idx = 0; buf_idx < buffer.size() && view_idx < file_view.size(); ++buf_idx, ++view_idx) {
      CHECK(file_view[view_idx] == buffer[buf_idx]);
    }

    mio::shared_mmap_source shared_file_view(std::move(file_view));
    CHECK(!file_view.is_open()); // NOLINT(bugprone-use-after-move)
    CHECK(shared_file_view.is_open());
    CHECK(shared_file_view.size() == buffer.size() - offset);
  }

  SUBCASE("test file can be mapped with offset above one page size") {
    std::error_code error;
    auto offset = mio::page_size() + 3;

    mio::mmap_source file_view = mio::make_mmap_source(path, offset, mio::map_entire_file, error);
    CHECK(!error);

    CHECK(file_view.is_open());
    CHECK(file_view.size() == buffer.size() - offset);

    for (size_t buf_idx = offset, view_idx = 0; buf_idx < buffer.size() && view_idx < file_view.size(); ++buf_idx, ++view_idx) {
      CHECK(file_view[view_idx] == buffer[buf_idx]);
    }

    mio::shared_mmap_source shared_file_view(std::move(file_view));
    CHECK(!file_view.is_open()); // NOLINT(bugprone-use-after-move)
    CHECK(shared_file_view.is_open());
    CHECK(shared_file_view.size() == buffer.size() - offset);
  }

  SUBCASE("test file can be mapped with offset above two more page sizes") {
    std::error_code error;
    auto offset = 2 * mio::page_size() + 3;

    mio::mmap_source file_view = mio::make_mmap_source(path, offset, mio::map_entire_file, error);
    CHECK(!error);

    CHECK(file_view.is_open());
    CHECK(file_view.size() == buffer.size() - offset);

    for (size_t buf_idx = offset, view_idx = 0; buf_idx < buffer.size() && view_idx < file_view.size(); ++buf_idx, ++view_idx) {
      CHECK(file_view[view_idx] == buffer[buf_idx]);
    }

    mio::shared_mmap_source shared_file_view(std::move(file_view));
    CHECK(!file_view.is_open()); // NOLINT(bugprone-use-after-move)
    CHECK(shared_file_view.is_open());
    CHECK(shared_file_view.size() == buffer.size() - offset);
  }

  SUBCASE("test mapping a non-empty but invalid file path results an error") {
    std::error_code error;
    mio::mmap_source m = mio::make_mmap_source("garbage-that-hopefully-doesnt-exist", 0, 0, error);
    CHECK(error);
    CHECK(m.empty());
    CHECK(!m.is_open());
  }

  SUBCASE("test mapping an empty C-style file path results an error") {
    std::error_code error;
    mio::mmap_source m = mio::make_mmap_source(static_cast<const char *>(nullptr), 0, 0, error);
    CHECK(error);
    CHECK(m.empty());
    CHECK(!m.is_open());
  }

  SUBCASE("test mapping an empty string type file path results an error") {
    std::error_code error;
    mio::mmap_source m = mio::make_mmap_source(std::string(), 0, 0, error);
    CHECK(error);
    CHECK(m.empty());
    CHECK(!m.is_open());
  }

  SUBCASE("test mapping an invalid file handle results an error") {
    std::error_code error;
    mio::mmap_source m = mio::make_mmap_source(mio::invalid_handle, 0, 0, error);
    CHECK(error);
    CHECK(m.empty());
    CHECK(!m.is_open());
  }

  SUBCASE("test mapping at an invalid offset results an error") {
    std::error_code error;
    mio::mmap_source m = mio::make_mmap_source(path, 100 * buffer.size(), buffer.size(), error);
    CHECK(error);
    CHECK(m.empty());
    CHECK(!m.is_open());
  }

  SUBCASE("test shared_mmap works as expected") {
    std::error_code error;

    mio::ummap_source _1;
    mio::shared_ummap_source _2;
    // Make sure shared_mmap mapping compiles as all testing was done on normal mmaps.
    mio::shared_mmap_source _3(path, 0, mio::map_entire_file);
    CHECK(_3.is_open());

    auto _4 = mio::make_mmap_source(path, error);
    CHECK(!error);

    auto _5 = mio::make_mmap<mio::shared_mmap_source>(path, 0, mio::map_entire_file, error);
    CHECK(!error);

#ifdef _WIN32
    const wchar_t *wpath1 = L"dasfsf";
    auto _6 = mio::make_mmap_source(wpath1, error);
    CHECK(error);

    mio::mmap_source _7;
    _7.map(wpath1, error);
    CHECK(error);

    const std::wstring wpath2 = wpath1;
    auto _8 = mio::make_mmap_source(wpath2, error);
    CHECK(error);

    mio::mmap_source _9;
    _9.map(wpath1, error);
    CHECK(error);
#else
    const int fd = open(path, O_RDONLY);
    mio::mmap_source _fdmmap(fd, 0, mio::map_entire_file);
    _fdmmap.unmap();
    _fdmmap.map(fd, error);
    CHECK(!error);
#endif
  }
}

// #define TEST_STATIC_ASSERT
TEST_CASE("csvdoc")
{
  using namespace mio::csv;

  SUBCASE("test QuotedCsvField can be specialized") {
    CHECK(QuotedCsvField<NAME("node_id")>::Quoted::value == true);
  }

  SUBCASE("test PlainCsvField can be specialized") {
    CHECK(PlainCsvField<NAME("node_id")>::Quoted::value == false);
  }

#ifdef TEST_STATIC_ASSERT
    SUBCASE("Csv field names cannot be duplicated") {
      CsvDocReader<
          Field<NAME("Field1")>,
          Field<NAME("Field1")>,
          Field<NAME("Field3")>,
          Field<NAME("Field4")>
      > doc;
    }
#endif

  SUBCASE("test CsvField has static field name") {
    auto field_name = Field<NAME("node_id")>::field_name;
    CHECK(std::strcmp(field_name, "node_id") == 0);
  }

  SUBCASE("test VerifyHeader returns success for valid column header") {
    CsvDoc<
        Field<NAME("Field1")>,
        Field<NAME("Field2")>,
        Field<NAME("Field3")>,
        Field<NAME("Field4")>
    > csv_doc;

    auto [success, msg] = csv_doc.VerifyHeader(std::string_view{"Field1,Field2,Field3,Field4"});
    CHECK(success);
  }

  SUBCASE("test VerifyHeader returns error for invalid column names") {
    CsvDoc<
        Field<NAME("Field1")>,
        Field<NAME("Field2")>,
        Field<NAME("Field3")>,
        Field<NAME("Field4")>
    > csv_doc;

    auto [success, msg] = csv_doc.VerifyHeader(std::string_view{"Field1,Field3,Field3,Field4"});
    CHECK(!success);
    CHECK(msg == "Invalid column names, code 0100");
  }

  SUBCASE("test VerifyHeader returns error for invalid column counts") {
    CsvDoc<
        Field<NAME("Field1")>,
        Field<NAME("Field2")>,
        Field<NAME("Field3")>,
        Field<NAME("Field4")>
    > csv_doc;

    auto [success, msg] = csv_doc.VerifyHeader(std::string_view{"Field1,Field2"});
    CHECK(!success);
  }

  SUBCASE("test CsvDoc returns correct number of fields") {
    CsvDoc<
        Field<NAME("Field1")>,
        Field<NAME("Field2")>,
        Field<NAME("Field3")>,
        Field<NAME("Field4")>
    > csv_doc;

    CHECK(csv_doc.field_count == 4);
  }

  SUBCASE("test make_record can convert csv line to fields") {
    using namespace std::literals;

    CsvDoc<
        Field<NAME("Field1")>,
        Field<NAME("Field2")>,
        QuotedField<NAME("Field3")>,
        Field<NAME("Field4")>
    > csv_doc;

    auto line = "1,2,\"hello,world\",6"sv;

    auto rec = csv_doc.make_record(line);
    CHECK(get<0>(rec).data.compare("1"sv) == 0);
    CHECK(get<2>(rec).data.compare("\"hello,world\""sv) == 0);
  }

}
