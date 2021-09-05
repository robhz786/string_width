# string_width

Header `string_width.hpp` implements string width calculation
[as specified](http://eel.is/c++draft/format.string.std#11)
for std::format.

```c++
namespace string_width {

enum class surrogate_policy : bool {
    strict = false, lax = true
};

struct width_and_pos {
    int width;
    std::size_t pos;
};

template <typename CharT>
width_and_pos str_width_and_pos
    ( int width_limit
    , const CharT* str
    , std::size_t str_len
    , surrogate_policy surr_poli = surrogate_policy::strict ) noexcept

template <typename CharT>
int str_width
    ( int width_limit
    , const CharT* str
    , std::size_t str_len
    , surrogate_policy surr_poli = surrogate_policy::strict ) noexcept

} // namespace string_width
```

Function `str_width_and_pos` returns a value `r` such that `r.pos`
is the size of the longest substring of `str` whose width is not
greater than `width_limit`, and `r.width` is the width of such
substring.

Function `str_width` returns only the width of such substring.
However, it may be a little bit faster than `str_width_and_pos`
in some situations.

`str` is expected to be encoded in UTF-8, UTF-16 or
UTF-32 (depending on `sizeof(CharT)`).

## Invalid sequences

The width of an invalid sequence
is calculated as being the number of replacements characters that
Unicode recommends to print when sanitizing such sequence.
For example, in UTF-8, the sequences `"\xE0\xA0"` ( missing continuation byte )
and `"\xE0\x9F\x80"` ( overlong sequence ) are both invalid and are expected to
be replaced by `"\uFFFD"` and `"\uFFFD\uFFFD\uFFFD"`, respectively. Thus, theirs widths
are calculated as `1` and `3`, respectively.

`surr_poli` controls whether invalid surrogates are considered invalid.
For example, in UTF-8, the sequence `"\xED\xA0\x80"` is invalid since it
encodes U+D800, which is a surrogate, and it is expected to be replaced
by `"\uFFFD\uFFFD\uFFFD"`. So its width is `3` when `surr_poli` is
`surrogate_policy::strict`. But if `surr_poli` is `surrogate_policy::lax`,
then such sequence is considered valid and its width is `1`.

