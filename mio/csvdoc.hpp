/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

#ifndef WXLIB_MIO_CSV_DOC_HPP
#define WXLIB_MIO_CSV_DOC_HPP

#include <mio/fastfind.hpp>

#include <bitset>
#include <ranges>

namespace mio::csv {

struct csv_field_tag_t
{
};

template<char...Cs>
struct CsvFieldTag
{
  using type = csv_field_tag_t;
  static constexpr std::array<char, sizeof...(Cs) + 1> field_name = {Cs..., 0};
};

template<typename T>
concept CsvFieldNameType = std::is_same_v<decltype(std::declval<T>()()), const char *>;

template<CsvFieldNameType T, std::size_t ...I>
constexpr auto name2tag(T name, std::index_sequence<I...>)
{
  return CsvFieldTag<name()[I]...>{};
}

template<CsvFieldNameType T>
constexpr auto name2tag(T name)
{
  constexpr auto strlen = [](const char *str) -> size_t {
    auto strlen_impl = [](const char *str, auto &strlen_ref) -> size_t {
      return str[0] == 0 ? 0 : strlen_ref(str + 1, strlen_ref) + 1;
    };

    return strlen_impl(str, strlen_impl);
  };

  return name2tag(name, std::make_index_sequence<strlen(name())>{});
}

#define forward_name(x) \
    []() constexpr { return x; }

#define NAME(x) \
    decltype(name2tag(forward_name(x)))

struct csv_field_t
{
};

template<typename T>
concept CsvFieldTagType = std::is_same_v<typename T::type, csv_field_tag_t>;

template<CsvFieldTagType T, bool quoted>
struct CsvField
{
  using type = csv_field_t;
  using Quoted = std::integral_constant<bool, quoted>;
  static constexpr const char *field_name = T::field_name.data();
  std::string_view data{nullptr, 0};
};

/*!
 * Example: QuotedCsvField<NAME("WKT")>
 */
template<typename T>
using QuotedCsvField = CsvField<T, true>;

/*!
 * Example: QuotedField<NAME("WKT")>
 */
template<typename T>
using QuotedField = QuotedCsvField<T>;

/*!
 * Example: PlainCsvField<NAME("node_id")>
 */
template<typename T>
using PlainCsvField = CsvField<T, false>;

/*!
 * Example: Field<NAME("WKT")>
 */
template<typename T>
using Field = PlainCsvField<T>;

template<typename T>
concept CsvFieldType = std::is_same_v<typename T::type, csv_field_t>;

template<CsvFieldType ...Ts>
using CsvRecord = std::tuple<Ts...>;

template<typename... Ts>
struct unique_csv_fields;

template<typename T1, typename T2, typename ... Ts>
struct unique_csv_fields<T1, T2, Ts...> : unique_csv_fields<T1, T2>, unique_csv_fields<T1, Ts...>, unique_csv_fields<T2, Ts...>
{
};

template<typename T1, typename T2>
struct unique_csv_fields<T1, T2>
{
  static_assert(!std::is_same<T1, T2>::value, "Csv fields must be unique.");
};

template<typename ...Ts>
concept UniqueCsvFields = requires {
  unique_csv_fields<Ts...>{};
};

template<typename ...Ts> requires UniqueCsvFields<Ts...>
struct CsvDoc
{
  using Record = CsvRecord<Ts...>;
  constexpr static auto numfields = std::tuple_size_v<Record>;

  CsvDoc() = default;
  CsvDoc(const CsvDoc &) = delete;
  CsvDoc(CsvDoc &&) = delete;
  ~CsvDoc() = default;
  CsvDoc &operator=(CsvDoc &) = delete;
  CsvDoc &operator=(CsvDoc &&) = delete;

  /*!
   * Verify the header line of the csv document.
   * @param a_header The first line of the csv document used as the header line.
   * @return {true, empty} for success, {false, err_message} for error. If any header column name does
   * not match the schema, the error message will output a bit string, with the corresponding bit set
   * to 1 (least significant bit on the left most position).
   */
  auto VerifyHeader(std::string_view a_header)
  {
    using namespace std;

    auto f = [&a_header](Ts const &... args) {
      // field names detected from the header.
      ranges::split_view detected_names{a_header, string_view{","}};
      auto it{detected_names.begin()};
      // compare the detected names to schema, and set the flag bit if different.
      bitset<sizeof...(args)> flags;
      size_t bitpos{0};
      ((flags.set(bitpos, string_view{args.field_name}.compare(string_view{*it}) != 0), bitpos++, it++), ...);

      auto s = flags.to_string();
      return flags.none() ?
             make_tuple(true, string("success")) :
             make_tuple(false, format("Invalid column names, error code {}", string{s.rbegin(), s.rend()}));
    };

    auto l_detected_numfields{std::count(a_header.begin(), a_header.end(), ',') + 1};
    return (l_detected_numfields == numfields) ?
           apply(f, Record{}) :
           make_tuple(false, format("Invalid column counts: expected {}, detected {} ", numfields, l_detected_numfields));
  }

  /*!
   * Makes a record using a comma separated string line.
   * Precondition - a_line must not be empty.
   * @param a_line Comma separated data line excluding `\n', possibly with comma enclosed in double quotes.
   * @return An CsvDoc::Record instance.
   */
  auto make_record(std::string_view a_line)
  {
    Record result{};
    const char *l_begin = a_line.data();
    const char *l_end = std::next(l_begin, a_line.size());
    make_record_impl(result, l_begin, l_end);
    return result;
  }

  /*!
   * Updates an existing record using a comma separated string line.
   * Precondition - a_line must not be empty.
   * @param a_rec Reference to an existing record.
   * @param a_line Comma separated data line excluding `\n', possibly with comma enclosed in double quotes.
   */
  void make_record(Record &a_rec, std::string_view a_line)
  {
    const char *l_begin = a_line.data();
    const char *l_end = std::next(l_begin, a_line.size());
    make_record_impl(a_rec, l_begin, l_end);
  }

  bool header_on_first_line{true};

private:
  template<size_t I = 0>
  void make_record_impl(Record &a_rec, const char *a_begin, const char *a_end)
  {
    if constexpr (I == numfields) {
      return;
    } else {
      const char *l_find{nullptr};

      if constexpr (std::remove_cvref_t<decltype(get<I>(a_rec))>::Quoted::value) {
        l_find = fast_find<'"'>(a_begin, a_end); // opening quote
        l_find = fast_find<'"'>(std::next(l_find), a_end); // closing quote
        l_find = fast_find<','>(l_find, a_end); // comma
      } else {
        l_find = fast_find<','>(a_begin, a_end);
      }

      std::get<I>(a_rec).data = {a_begin, l_find};
      return l_find != a_end ? a_begin = std::next(l_find), make_record_impl<I + 1>(a_rec, a_begin, a_end) : void();
    }
  }
};

}
#endif