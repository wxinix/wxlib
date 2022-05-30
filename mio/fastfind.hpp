/*!
  MPL 1.1/GPL 2.0/LGPL 2.1 tri-license
  Copyright (C) 2022  Wuping Xin
*/

#include <bit>
#include <immintrin.h>

#ifndef WXLIB_MIO_FAST_FIND_HPP
#define WXLIB_MIO_FAST_FIND_HPP

namespace mio {

/**
 * Finds the end position of the first char counted backwards in [a_begin, a_begin + a_size).
 * @param a_begin - The first_iter of the memory search range.
 * @param a_size - The size of memory search range.
 * @return The position just passing the first `\n` counted from the back of the span.
 * @remarks The span is defined by [first, first_iter + size).
 */
template<unsigned char C>
static const char *find_end(const char *a_begin, size_t a_size) noexcept
{
  using iter_diff_t = typename std::iterator_traits<const char *>::difference_type;
  auto rb = std::reverse_iterator(std::next(a_begin, static_cast<iter_diff_t>(a_size)));
  auto re = std::reverse_iterator(a_begin);

  return std::find(rb, re, C).base();
}

/*!
 * Finds the char using AVX2 intrinsics in 32 byte step.
 * Best for searching a region larger than 32 bytes.
 * @tparam C
 * @param a_begin
 * @param a_end
 * @return
 */
template<unsigned char C>
static const char *avx2_find(const char *a_begin, const char *a_end)
{
  const char *begin = a_begin;

  for (auto q = _mm256_set1_epi8(C); begin + 32 < a_end; begin += 32) {
    auto x = _mm256_lddqu_si256(reinterpret_cast<const __m256i *>(begin));
    auto r = _mm256_cmpeq_epi8(x, q);
    auto z = _mm256_movemask_epi8(r);

    if (z) {
#ifdef __GNUC__
      const char *rr = begin + __builtin_ffs(z) - 1;
#else
      unsigned long b;
      auto rr = _BitScanForward(&b, z) ? (begin + b) : begin;
#endif
      return rr < a_end ? rr : a_end;
    }
  }

  return oct_find<C>(begin, a_end);
}

/*!
 * Finds the target char in 8 bytes step.
 * @tparam C
 * @param a_begin
 * @param a_end
 * @return
 */
template<unsigned char C>
static const char *oct_find(const char *a_begin, const char *a_end)
{
  constexpr uint64_t k = C;
  constexpr uint64_t p = k | (k << 0x08) | (k << 0x10) | (k << 0x18) | (k << 0x20) | (k << 0x28) | (k << 0x30) | (k << 0x38);

  auto do_find = [&](const uint64_t *a_data) {
    auto input = (*a_data) ^ p;
    auto tmp = (input & 0x7F7F7F7F7F7F7F7FL) + 0x7F7F7F7F7F7F7F7FL;
    tmp = ~(tmp | input | 0x7F7F7F7F7F7F7F7FL);
    return std::countr_zero(tmp) >> 3;
  };

  const char *b = a_begin;
  auto jmp = std::distance(a_begin, a_end) >> 3;
  
  for (int i = 0; i < jmp; i++) {
    auto find_pos = do_find((uint64_t *) (b));
    if (find_pos == 8)
      b = std::next(b, 8);
    else
      return std::next(b, find_pos);
  }

  return std::find(b, a_end, C);
}

template<unsigned char C>
static const char *fast_find(const char *a_begin, const char *a_end) noexcept
{
#ifdef __AVX2__
  return avx2_find<C>(a_begin, a_end);
#elif __GNUC__
  return oct_find<C>(a_begin, a_end);
#else
  return std::find(a_begin, a_end, C);
#endif
}

}

#endif