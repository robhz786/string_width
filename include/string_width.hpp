#ifndef STRING_WIDTH_HPP
#define STRING_WIDTH_HPP

//  Copyright (C) (See commit logs on github.com/robhz786/strf)
//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <type_traits>
#include <cstddef>
#include <cstdint>

#if ! defined(STRING_WIDTH_ASSERT)
#  if ! defined(STRING_WIDTH_FREESTANDING) && defined(__STDC_HOSTED__) && __STDC_HOSTED__ == 1
#    include <cassert>
#    define STRING_WIDTH_ASSERT(x) assert(x)
#  else
#    define STRING_WIDTH_ASSERT(x)
#  endif
#endif // ! defined(STRING_WIDTH_ASSERT)

#if defined(STRING_WIDTH_SOURCE) && !defined(STRING_WIDTH_SEPARATE_COMPILATION)
#define STRING_WIDTH_SEPARATE_COMPILATION
#endif

#if defined(STRING_WIDTH_SOURCE)
// When building static library
#  define STRING_WIDTH_FUNC_IMPL
#elif defined(STRING_WIDTH_SEPARATE_COMPILATION)
// When using static library
#  define STRING_WIDTH_OMIT_IMPL
#else
// When using header-only library
#  define STRING_WIDTH_FUNC_IMPL inline
#endif

#if defined(__GNUC__) || defined (__clang__)
#  define STRING_WIDTH_IF_LIKELY(x)   if(__builtin_expect(!!(x), 1))
#  define STRING_WIDTH_IF_UNLIKELY(x) if(__builtin_expect(!!(x), 0))

#elif defined(_MSC_VER) && (_MSC_VER >= 1926) && (_MSVC_LANG > 201703L)
#  define STRING_WIDTH_IF_LIKELY(x)   if(x)   [[likely]]
#  define STRING_WIDTH_IF_UNLIKELY(x) if(x) [[unlikely]]

#else
#  define STRING_WIDTH_IF_LIKELY(x)   if(x)
#  define STRING_WIDTH_IF_UNLIKELY(x) if(x)
#endif

#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Warray-bounds"
#endif

#define STRING_WIDTH_CHECK_DEST                         \
    STRING_WIDTH_IF_UNLIKELY (dest_it == dest_end) {    \
        dest.advance_to(dest_it);                       \
        dest.recycle();                                 \
        STRING_WIDTH_IF_UNLIKELY (!dest.good()) {       \
            return;                                     \
        }                                               \
        dest_it = dest.pointer();                       \
        dest_end = dest.end();                          \
    }

#define STRING_WIDTH_CHECK_DEST_SIZE(SIZE)                  \
    STRING_WIDTH_IF_UNLIKELY (dest_it + SIZE > dest_end) {  \
        dest.advance_to(dest_it);                           \
        dest.recycle();                                     \
        STRING_WIDTH_IF_UNLIKELY (!dest.good()) {           \
            return;                                         \
        }                                                   \
        dest_it = dest.pointer();                           \
        dest_end = dest.end();                              \
    }


namespace string_width {

enum class surrogate_policy : bool {
    strict = false, lax = true
};

using width_t = int;

namespace detail {

#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Warray-bounds"
#endif

template <typename CharT>
class destination
{
public:

    using char_type = CharT;

    destination(const destination&) = delete;
    destination(destination&&) = delete;
    destination& operator=(const destination&) = delete;
    destination& operator=(destination&&) = delete;

    virtual ~destination() { };

    char_type* pointer() const noexcept
    {
        return pointer_;
    }
    char_type* end() const noexcept
    {
        return end_;
    }
    std::size_t space() const noexcept
    {
        STRING_WIDTH_ASSERT(pointer_ <= end_);
        return end_ - pointer_;
    }
    bool good() const noexcept
    {
        return good_;
    }
    void advance_to(char_type* p) noexcept
    {
        STRING_WIDTH_ASSERT(pointer_ <= p);
        STRING_WIDTH_ASSERT(p <= end_);
        pointer_ = p;
    }
    void advance(std::size_t n) noexcept
    {
        STRING_WIDTH_ASSERT(pointer() + n <= end());
        pointer_ += n;
    }
    void advance() noexcept
    {
        STRING_WIDTH_ASSERT(pointer() < end());
        ++pointer_;
    }
    void require(std::size_t s) noexcept
    {
        STRING_WIDTH_IF_UNLIKELY (pointer() + s > end()) {
            recycle();
        }
        STRING_WIDTH_ASSERT(pointer() + s <= end());
    }
    void ensure(std::size_t s) noexcept
    {
        require(s);
    }

    virtual void recycle()  noexcept = 0;

protected:

    destination(char_type* p, char_type* e) noexcept
        : pointer_(p), end_(e)
    { }

    destination(char_type* p, std::size_t s) noexcept
        : pointer_(p), end_(p + s)
    { }

    void set_pointer(char_type* p) noexcept
    { pointer_ = p; };
    void set_end(char_type* e) noexcept
    { end_ = e; };
    void set_good(bool g) noexcept
    { good_ = g; };

private:

    char_type* pointer_;
    char_type* end_;
    bool good_ = true;
};

#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#  pragma GCC diagnostic ignored "-Warray-bounds"
#endif

constexpr bool is_high_surrogate(std::uint32_t codepoint) noexcept
{
    return codepoint >> 10 == 0x36;
}
constexpr bool is_low_surrogate(std::uint32_t codepoint) noexcept
{
    return codepoint >> 10 == 0x37;
}
constexpr bool not_surrogate(std::uint32_t codepoint) noexcept
{
    return codepoint >> 11 != 0x1B;
}
constexpr  bool not_high_surrogate(std::uint32_t codepoint) noexcept
{
    return codepoint >> 10 != 0x36;
}
constexpr  bool not_low_surrogate(std::uint32_t codepoint) noexcept
{
    return codepoint >> 10 != 0x37;
}
constexpr std::uint16_t utf8_decode(std::uint16_t ch0, std::uint16_t ch1) noexcept
{
    return (((ch0 & 0x1F) << 6) |
            ((ch1 & 0x3F) << 0));
}
constexpr std::uint16_t utf8_decode(std::uint16_t ch0, std::uint16_t ch1, std::uint16_t ch2) noexcept
{
    return (((ch0 & 0x0F) << 12) |
            ((ch1 & 0x3F) <<  6) |
            ((ch2 & 0x3F) <<  0));
}
constexpr std::uint32_t utf8_decode(std::uint32_t ch0, std::uint32_t ch1, std::uint32_t ch2, std::uint32_t ch3) noexcept
{
    return (((ch0 & 0x07) << 18) |
            ((ch1 & 0x3F) << 12) |
            ((ch2 & 0x3F) <<  6) |
            ((ch3 & 0x3F) <<  0));
}
inline unsigned utf8_decode_first_2_of_3(std::uint8_t ch0, std::uint8_t ch1) noexcept
{
    return ((ch0 & 0x0F) << 6) | (ch1 & 0x3F);
}
constexpr bool is_utf8_continuation(std::uint8_t ch) noexcept
{
    return (ch & 0xC0) == 0x80;
}

inline bool first_2_of_3_are_valid(unsigned x, string_width::surrogate_policy surr_poli) noexcept
{
    return ( surr_poli == string_width::surrogate_policy::lax
          || (x >> 5) != 0x1B );
}
inline bool first_2_of_3_are_valid
    ( std::uint8_t ch0
    , std::uint8_t ch1
    , string_width::surrogate_policy surr_poli ) noexcept
{
    return first_2_of_3_are_valid(utf8_decode_first_2_of_3(ch0, ch1), surr_poli);
}

inline unsigned utf8_decode_first_2_of_4(std::uint8_t ch0, std::uint8_t ch1) noexcept
{
    return ((ch0 ^ 0xF0) << 6) | (ch1 & 0x3F);
}

inline unsigned utf8_decode_last_2_of_4(unsigned long x, unsigned ch2, unsigned ch3) noexcept
{
    return (x << 12) | ((ch2 & 0x3F) <<  6) | (ch3 & 0x3F);
}

inline bool first_2_of_4_are_valid(unsigned x) noexcept
{
    return 0xF < x && x < 0x110;
}

inline bool first_2_of_4_are_valid(std::uint8_t ch0, std::uint8_t ch1) noexcept
{
    return first_2_of_4_are_valid(utf8_decode_first_2_of_4(ch0, ch1));
}

template <typename SrcCharT, typename DestCharT>
typename std::enable_if<sizeof(SrcCharT) == 1, void>::type decode
    ( string_width::detail::destination<DestCharT>& dest
    , const SrcCharT* src
    , std::size_t src_size
    , string_width::surrogate_policy surr_poli ) noexcept
{
    using string_width::detail::utf8_decode;
    using string_width::detail::utf8_decode_first_2_of_3;
    using string_width::detail::utf8_decode_first_2_of_4;
    using string_width::detail::utf8_decode_last_2_of_4;
    using string_width::detail::first_2_of_3_are_valid;
    using string_width::detail::first_2_of_4_are_valid;
    using string_width::detail::is_utf8_continuation;

    std::uint8_t ch0, ch1, ch2, ch3;
    unsigned long x;
    auto src_it = src;
    auto src_end = src + src_size;
    auto dest_it = dest.pointer();
    auto dest_end = dest.end();
    DestCharT ch32;

    while(src_it != src_end) {
        ch0 = (*src_it);
        ++src_it;
        if (ch0 < 0x80) {
            ch32 = ch0;
        } else if (0xC0 == (ch0 & 0xE0)) {
            if(ch0 > 0xC1 && src_it != src_end && is_utf8_continuation(ch1 = * src_it)) {
                ch32 = utf8_decode(ch0, ch1);
                ++src_it;
            } else goto invalid_sequence;
        } else if (0xE0 == ch0) {
            if (   src_it != src_end && (((ch1 = * src_it) & 0xE0) == 0xA0)
              && ++src_it != src_end && is_utf8_continuation(ch2 = * src_it) )
            {
                ch32 = ((ch1 & 0x3F) << 6) | (ch2 & 0x3F);
                ++src_it;
            } else goto invalid_sequence;
        } else if (0xE0 == (ch0 & 0xF0)) {
            if (   src_it != src_end && is_utf8_continuation(ch1 = * src_it)
              && first_2_of_3_are_valid( x = utf8_decode_first_2_of_3(ch0, ch1)
                                       , surr_poli )
              && ++src_it != src_end && is_utf8_continuation(ch2 = * src_it) )
            {
                ch32 = (x << 6) | (ch2 & 0x3F);
                ++src_it;
            } else goto invalid_sequence;
        } else if ( src_it != src_end
                 && is_utf8_continuation(ch1 = * src_it)
                 && first_2_of_4_are_valid(x = utf8_decode_first_2_of_4(ch0, ch1))
                 && ++src_it != src_end && is_utf8_continuation(ch2 = * src_it)
                 && ++src_it != src_end && is_utf8_continuation(ch3 = * src_it) )
        {
            ch32 = utf8_decode_last_2_of_4(x, ch2, ch3);
            ++src_it;
        } else {
            invalid_sequence:
            ch32 = 0xFFFD;
        }

        STRING_WIDTH_CHECK_DEST;
        *dest_it = ch32;
        ++dest_it;
    }
    dest.advance_to(dest_it);
}

template <typename SrcCharT, typename DestCharT>
typename std::enable_if<sizeof(SrcCharT) == 2, void>::type decode
    ( string_width::detail::destination<DestCharT>& dest
    , const SrcCharT* src
    , std::size_t src_size
    , string_width::surrogate_policy surr_poli ) noexcept
{
    unsigned long ch, ch2;
    DestCharT ch32;
    const SrcCharT* src_it_next;
    auto src_end = src + src_size;
    auto dest_it = dest.pointer();
    auto dest_end = dest.end();
    for(auto src_it = src; src_it != src_end; src_it = src_it_next) {
        src_it_next = src_it + 1;
        ch = *src_it;
        src_it_next = src_it + 1;

        STRING_WIDTH_IF_LIKELY (string_width::detail::not_surrogate(ch)) {
            ch32 = ch;
        } else if ( string_width::detail::is_high_surrogate(ch)
               && src_it_next != src_end
               && string_width::detail::is_low_surrogate(ch2 = *src_it_next)) {
            ch32 = 0x10000 + (((ch & 0x3FF) << 10) | (ch2 & 0x3FF));
            ++src_it_next;
        } else if (surr_poli == string_width::surrogate_policy::lax) {
            ch32 = ch;
        } else {
            ch32 = 0xFFFD;
        }

        STRING_WIDTH_CHECK_DEST;
        *dest_it = ch32;
        ++dest_it;
    }
    dest.advance_to(dest_it);
}

template <typename SrcCharT, typename DestCharT>
typename std::enable_if<sizeof(SrcCharT) == 4, void>::type decode
    ( string_width::detail::destination<DestCharT>& dest
    , const SrcCharT* src
    , std::size_t src_size
    , string_width::surrogate_policy ) noexcept
{
    // to-do : optimize ( not need to sanitize )

    const auto src_end = src + src_size;
    auto dest_it = dest.pointer();
    auto dest_end = dest.end();
    for (auto src_it = src; src_it < src_end; ++src_it) {
        auto ch = *src_it;
        STRING_WIDTH_IF_UNLIKELY (ch >= 0x110000) {
            ch = 0xFFFD;
        }
        STRING_WIDTH_CHECK_DEST;
        *dest_it = ch;
        ++dest_it;
    }
    dest.advance_to(dest_it);
}

struct codepoints_count_result {
    std::size_t count;
    std::size_t pos;
};

template <typename CharT>
typename std::enable_if
    < sizeof(CharT) == 1
    , string_width::detail::codepoints_count_result >::type
count_codepoints
    ( const CharT* src
    , std::size_t src_size
    , std::size_t max_count
    , string_width::surrogate_policy surr_poli ) noexcept
{
    using string_width::detail::utf8_decode;
    using string_width::detail::is_utf8_continuation;
    using string_width::detail::first_2_of_3_are_valid;
    using string_width::detail::first_2_of_4_are_valid;

    std::uint8_t ch0, ch1;
    std::size_t count = 0;
    auto it = src;
    auto end = src + src_size;
    while (it != end && count != max_count) {
        ch0 = (*it);
        ++it;
        ++count;
        if (0xC0 == (ch0 & 0xE0)) {
            if (ch0 > 0xC1 && it != end && is_utf8_continuation(*it)) {
                ++it;
            }
        } else if (0xE0 == ch0) {
            if (   it != end && ((*it & 0xE0) == 0xA0)
              && ++it != end && is_utf8_continuation(*it) )
            {
                ++it;
            }
        } else if (0xE0 == (ch0 & 0xF0)) {
            if ( it != end && is_utf8_continuation(ch1 = *it)
              && first_2_of_3_are_valid( ch0, ch1, surr_poli )
              && ++it != end && is_utf8_continuation(*it) )
            {
                ++it;
            }
        } else if (   it != end && is_utf8_continuation(ch1 = * it)
                 && first_2_of_4_are_valid(ch0, ch1)
                 && ++it != end && is_utf8_continuation(*it)
                 && ++it != end && is_utf8_continuation(*it) )
        {
            ++it;
        }
    }
    return {count, static_cast<std::size_t>(it - src)};
}

template <typename CharT>
typename std::enable_if
    < sizeof(CharT) == 2
    , string_width::detail::codepoints_count_result >::type
count_codepoints
    ( const CharT* src
    , std::size_t src_size
    , std::size_t max_count
    , string_width::surrogate_policy ) noexcept
{
    std::size_t count = 0;
    const CharT* it = src;
    const auto end = src + src_size;
    unsigned long ch;
    while (it != end && count < max_count) {
        ch = *it;
        ++ it;
        ++ count;
        if ( string_width::detail::is_high_surrogate(ch) && it != end
          && string_width::detail::is_low_surrogate(*it)) {
            ++ it;
        }
    }
    return {count, static_cast<std::size_t>(it - src)};
}

template <typename CharT>
typename std::enable_if
    < sizeof(CharT) == 4
    , string_width::detail::codepoints_count_result >::type
count_codepoints
    ( const CharT*
    , std::size_t src_size
    , std::size_t max_count
    , string_width::surrogate_policy ) noexcept
{
    if (max_count <= src_size) {
        return {max_count, max_count};
    }
    return {src_size, src_size};
}


#if ! defined(STRING_WIDTH_OMIT_IMPL)

struct std_width_calc_func_return {

    std_width_calc_func_return
        ( string_width::width_t width_
        , unsigned state_
        , const char32_t* ptr_ ) noexcept
        : width(width_)
        , state(state_)
        , ptr(ptr_)
    {
    }

    std_width_calc_func_return(const std_width_calc_func_return&) = default;

    string_width::width_t width;
    unsigned state;
    const char32_t* ptr;
};

STRING_WIDTH_FUNC_IMPL std_width_calc_func_return std_width_calc_func
    ( const char32_t* str
    , const char32_t* end
    , string_width::width_t width
    , unsigned state
    , bool return_pos ) noexcept
{
    // following http://www.unicode.org/reports/tr29/tr29-37.html#Grapheme_Cluster_Boundaries

    using state_t = unsigned;
    constexpr state_t initial          = 0;
    constexpr state_t after_prepend    = 1;
    constexpr state_t after_core       = 1 << 1;
    constexpr state_t after_ri         = after_core | (1 << 2);
    constexpr state_t after_xpic       = after_core | (1 << 3);
    constexpr state_t after_xpic_zwj   = after_core | (1 << 4);
    constexpr state_t after_hangul     = after_core | (1 << 5);
    constexpr state_t after_hangul_l   = after_hangul | (1 << 6);
    constexpr state_t after_hangul_v   = after_hangul | (1 << 7);
    constexpr state_t after_hangul_t   = after_hangul | (1 << 8);
    constexpr state_t after_hangul_lv  = after_hangul | (1 << 9);
    constexpr state_t after_hangul_lvt = after_hangul | (1 << 10);
    constexpr state_t after_poscore    = 1 << 11;
    constexpr state_t after_cr         = 1 << 12;

    string_width::width_t ch_width;
    char32_t ch;
    goto next_codepoint;

    handle_other:
    if (state == after_prepend) {
        state = after_core;
        goto next_codepoint;
    }
    state = after_core;

    decrement_width:
    // should come here after the first codepoint of every grapheme cluster
    if (ch_width >= width) {
        if (! return_pos) {
            return {0, 0, nullptr};
        }
        if (ch_width > width) {
            return {0, 0, str -1};
        }
        width = 0;
        goto next_codepoint; // because there might be more codepoints in this grapheme cluster
    }
    width -= ch_width;

    next_codepoint:
    if (str == end) {
        return {width, state, str};
    }
    ch = *str;
    ++str;
    ch_width = 1;
    if (ch <= 0x007E) {
        if (0x20 <= ch) {
            ch_width = 1;
            goto handle_other;
        }
        if (0x000D == ch) { // CR
            goto handle_cr;
        }
        if (0x000A == ch) { // LF
            goto handle_lf;
        }
        goto handle_control;
    }

#include <string_width/detail/ch32_width_and_gcb_prop>

    handle_zwj:
    if (state == after_xpic) {
        state = after_xpic_zwj;
        goto next_codepoint;
    }
    goto handle_spacing_mark; // because the code is the same

    handle_extend:
    handle_extend_and_control:
    if (state == after_xpic) {
        goto next_codepoint;
    }

    handle_spacing_mark:
    if (state & (after_prepend | after_core | after_poscore)) {
        state = after_poscore;
        goto next_codepoint;
    }
    state = after_poscore;
    goto decrement_width;

    handle_prepend:
    if (state == after_prepend) {
        goto next_codepoint;
    }
    state = after_prepend;
    goto decrement_width;

    handle_regional_indicator: {
        if (state == after_ri) {
            state = after_core;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_ri;
            goto decrement_width;
        }
        state = after_ri;
        goto next_codepoint;
    }
    handle_extended_picto: {
        if (state == after_xpic_zwj) {
            state = after_xpic;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_xpic;
            goto decrement_width;
        }
        state = after_xpic;
        goto next_codepoint;
    }
    handle_hangul_l: {
        if (state == after_hangul_l) {
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_hangul_l;
            goto decrement_width;
        }
        state = after_hangul_l;
        goto next_codepoint;
    }
    handle_hangul_v: {
        constexpr state_t mask = ~after_hangul &
            (after_hangul_l | after_hangul_v | after_hangul_lv);
        if (state & mask) {
            state = after_hangul_v;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_hangul_v;
            goto decrement_width;
        }
        state = after_hangul_v;
        goto next_codepoint;
    }
    handle_hangul_t: {
        constexpr state_t mask = ~after_hangul &
            (after_hangul_v | after_hangul_lv | after_hangul_lvt | after_hangul_t);
        if (state & mask) {
            state = after_hangul_t;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_hangul_t;
            goto decrement_width;
        }
        state = after_hangul_t;
        goto next_codepoint;
    }
    handle_hangul_lv_or_lvt:
    if ( ch <= 0xD788 // && 0xAC00 <= ch
         && 0 == (ch & 3)
         && 0 == ((ch - 0xAC00) >> 2) % 7)
    {   // LV
        if (state == after_hangul_l) {
            state = after_hangul_lv;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_hangul_lv;
            goto decrement_width;
        }
        state = after_hangul_lv;
        goto next_codepoint;

    } else { // LVT
        if (state == after_hangul_l) {
            state = after_hangul_lvt;
            goto next_codepoint;
        }
        if (state != after_prepend) {
            state = after_hangul_lvt;
            goto decrement_width;
        }
        state = after_hangul_lvt;
        goto next_codepoint;
    }

    handle_cr:
    state = after_cr;
    goto decrement_width;

    handle_lf:
    if (state == after_cr) {
        state = initial;
        goto next_codepoint;
    }
    handle_control:
    state = initial;
    goto decrement_width;
}

#else

std_width_calc_func_return std_width_calc_func
    ( const char32_t* begin
    , const char32_t* end
    , string_width::width_t width
    , unsigned state
    , bool return_pos ) noexcept;

#endif // ! defined(STRING_WIDTH_OMIT_IMPL)

#if defined(__GNUC__) && (__GNUC__ >= 11)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif


class std_width_decrementer: public string_width::detail::destination<char32_t> {
public:
    std_width_decrementer (string_width::width_t initial_width) noexcept
        : string_width::detail::destination<char32_t>(buff_, buff_size_)
        , width_{initial_width}
    {
        this->set_good(initial_width != 0);
    }

    void recycle() noexcept override {
        if (this->good()) {
            auto res = detail::std_width_calc_func(buff_, this->pointer(), width_, state_, false);
            width_ = res.width;
            state_ = res.state;
            if (width_ == 0) {
                this->set_good(false);
            }
        }
        this->set_pointer(buff_);
    }

    string_width::width_t get_remaining_width()  noexcept {
        if (width_ != 0 && this->pointer() != buff_) {
            auto res = detail::std_width_calc_func(buff_, this->pointer(), width_, state_, false);
            return res.width;
        }
        return width_;
    }

private:
    string_width::width_t width_;
    unsigned state_ = 0;
    static constexpr std::size_t buff_size_ = 16;
    char32_t buff_[buff_size_];
};

class std_width_decrementer_with_pos: public string_width::detail::destination<char32_t> {
public:
    std_width_decrementer_with_pos (string_width::width_t initial_width) noexcept
        : string_width::detail::destination<char32_t>(buff_, buff_size_)
        , width_{initial_width}
    {
        this->set_good(initial_width != 0);
    }

    void recycle() noexcept override {
        if (this->good()) {
            auto res = detail::std_width_calc_func(buff_, this->pointer(), width_, state_, true);
            width_ = res.width;
            state_ = res.state;
            codepoints_count_ += (res.ptr - buff_);
            if (width_ == 0 && res.ptr != this->pointer()) {
                this->set_good(false);
            }
        }
        this->set_pointer(buff_);
    }

    struct result {
        string_width::width_t remaining_width;
        bool whole_string_covered;
        std::size_t codepoints_count;
    };

    result get_remaining_width_and_codepoints_count() noexcept {
        if (! this->good()) {
            return {0, false, codepoints_count_};
        }
        auto res = detail::std_width_calc_func(buff_, this->pointer(), width_, state_, true);
        width_ = res.width;
        codepoints_count_ += (res.ptr - buff_);
        bool whole_string_covered = (res.ptr == this->pointer());
        return {width_, whole_string_covered, codepoints_count_};
    }

private:
    string_width::width_t width_;
    unsigned state_ = 0;
    std::size_t codepoints_count_ = 0;
    static constexpr std::size_t buff_size_ = 16;
    char32_t buff_[buff_size_];
};

} // namespace detail

template <typename CharT>
string_width::width_t str_width
    ( string_width::width_t limit
    , const CharT* str
    , std::size_t str_len
    , string_width::surrogate_policy surr_poli = string_width::surrogate_policy::strict ) noexcept
{
    detail::std_width_decrementer decr{limit};
    detail::decode(decr, str, str_len, surr_poli);
    return (limit - decr.get_remaining_width());
}

struct width_and_pos {
    string_width::width_t width;
    std::size_t pos;
};

template <typename CharT>
string_width::width_and_pos str_width_and_pos
    ( string_width::width_t limit
    , const CharT* str
    , std::size_t str_len
    , string_width::surrogate_policy surr_poli = string_width::surrogate_policy::strict ) noexcept
{
    string_width::detail::std_width_decrementer_with_pos decr{limit};
    string_width::detail::decode(decr, str, str_len, surr_poli);
    auto res = decr.get_remaining_width_and_codepoints_count();

    string_width::width_t width = limit - res.remaining_width;
    if (res.whole_string_covered) {
        return {width, str_len};
    }
    auto res2 = string_width::detail::count_codepoints
        (str, str_len, res.codepoints_count, surr_poli);
    return {width, res2.pos};
}

} // namespace string_width

#undef STRING_WIDTH_ASSERT
#undef STRING_WIDTH_CHECK_DEST
#undef STRING_WIDTH_CHECK_DEST_SIZE
#undef STRING_WIDTH_FUNC_IMPL
#undef STRING_WIDTH_IF_LIKELY
#undef STRING_WIDTH_IF_UNLIKELY
#undef STRING_WIDTH_OMIT_IMPL
#undef STRING_WIDTH_SEPARATE_COMPILATION

#endif // STRING_WIDTH_HPP
