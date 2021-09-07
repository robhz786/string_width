//  Distributed under the Boost Software License, Version 1.0.
//  (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <unicode/ucnv.h>
#include <unicode/stringpiece.h>
#include <unicode/unistr.h>

#include <vector>
#include <map>
#include <strf/to_cfile.hpp>
#include "unicode/uchar.h"

// https://unicode.org/reports/tr29/#Grapheme_Cluster_Boundaries
// https://unicode-org.github.io/icu-docs/apidoc/dev/icu4c/uchar_8h.html

constexpr int indentation_factor = 4;

enum class category {

    invalid = 0,
    other,
    extend,
    control,
    extend_and_control,
    spacing_mark,
    prepend,
    hangul_l,
    hangul_v,
    hangul_t,
    hangul_lv_or_lvt,
    regional_indicator,
    extended_picto,
    zwj,

    dw_mask  = 1 << 8, // this tells that the width is 2

    other_dw =                other              | dw_mask,
    extend_dw =               extend             | dw_mask,
    control_dw =              control            | dw_mask,
    extend_and_control_dw =   extend_and_control | dw_mask,
    spacing_mark_dw =         spacing_mark       | dw_mask,
    prepend_dw =              prepend            | dw_mask,
    hangul_l_dw =             hangul_l           | dw_mask,
    hangul_v_dw =             hangul_v           | dw_mask,
    hangul_t_dw =             hangul_t           | dw_mask,
    hangul_lv_or_lvt_dw =     hangul_lv_or_lvt   | dw_mask,
    regional_indicator_dw =   regional_indicator | dw_mask,
    extended_picto_dw =       extended_picto     | dw_mask,
};

constexpr bool is_double_width(category cat) {
    return ((int)cat & (int)category::dw_mask) != 0;
}

constexpr category remove_dw_flag(category cat) {
    return static_cast<category>((int)cat & ~(int)category::dw_mask);
}


category operator|(int m, category c)
{
    return static_cast<category>(m | (int)c);
}

const char* to_str(category cat) {
    switch(cat) {
        case category::other: return "other";
        case category::extend: return "extend";
        case category::control: return "control";
        case category::extend_and_control: return "extend_and_control";
        case category::spacing_mark: return "spacing_mark";
        case category::prepend: return "prepend";
        case category::hangul_l: return "hangul_l";
        case category::hangul_v: return "hangul_v";
        case category::hangul_t: return "hangul_t";
        case category::hangul_lv_or_lvt: return "hangul_lv_or_lvt";
        case category::regional_indicator: return "regional_indicator";
        case category::extended_picto: return "extended_picto";
        case category::zwj: return "zwj";
        case category::other_dw: return "other_dw";
        case category::extend_dw: return "extend_dw";
        case category::control_dw: return "control_dw";
        case category::extend_and_control_dw: return "extend_and_control_dw";
        case category::spacing_mark_dw: return "spacing_mark_dw";
        case category::prepend_dw: return "prepend_dw";
        case category::hangul_l_dw: return "hangul_l_dw";
        case category::hangul_v_dw: return "hangul_v_dw";
        case category::hangul_t_dw: return "hangul_t_dw";
        case category::hangul_lv_or_lvt_dw: return "hangul_lv_or_lvt_dw";
        case category::regional_indicator_dw: return "regional_indicator_dw";
        case category::extended_picto_dw: return "extended_picto_dw";
    }
    return "INVALID_CATEGORY";
}

bool is_zwj(UChar32 ch) {
    return ch == 0x200D;
}
bool is_extend(UChar32 ch) {
    return u_getIntPropertyValue(ch, UCHAR_GRAPHEME_EXTEND)
        || u_getIntPropertyValue(ch, UCHAR_EMOJI_MODIFIER);
}
bool is_extended_picto(UChar32 ch) {
    return u_getIntPropertyValue(ch, UCHAR_EXTENDED_PICTOGRAPHIC);
}
bool is_regional_indicator(UChar32 ch) {
    return u_getIntPropertyValue(ch, UCHAR_REGIONAL_INDICATOR);
}
bool is_spacing_mark(UChar32 ch) {
    auto gra_clu_brk = static_cast<UGraphemeClusterBreak>
        ( u_getIntPropertyValue(ch, UCHAR_GRAPHEME_CLUSTER_BREAK) );

    if (gra_clu_brk != U_GCB_EXTEND) {

        if (ch == 0x0E33 || ch == 0x0EB3) {
            return true;
        }
        auto gencat = static_cast<UCharCategory>
            (u_getIntPropertyValue(ch, UCHAR_GENERAL_CATEGORY));

        if ( gencat == U_COMBINING_SPACING_MARK
          && ch != 0x102B
          && ch != 0x102C
          && ch != 0x1038
          && ! (0x1062 <= ch && ch <= 0x1064)
          && ! (0x1067 <= ch && ch <= 0x106D)
          && ch != 0x1083
          && ! (0x1087 <= ch && ch <= 0x108C)
          && ch != 0x108F
          && ! (0x109A <= ch && ch <= 0x109C)
          && ch != 0x1A61
          && ch != 0x1A63
          && ch != 0x1A64
          && ch != 0xAA7B
          && ch != 0xAA7D
          && ch != 0x11720
          && ch != 0x11721 )
        {
            return true;
        }
    }
    return false;
}
bool is_prepend(UChar32 ch) {
    if (u_getIntPropertyValue(ch, UCHAR_PREPENDED_CONCATENATION_MARK))
        return true;

    auto insc = static_cast<UIndicSyllabicCategory>
        (u_getIntPropertyValue(ch, UCHAR_INDIC_SYLLABIC_CATEGORY));

    return insc == U_INSC_CONSONANT_PRECEDING_REPHA
        || insc == U_INSC_CONSONANT_PREFIXED;
}
bool is_hangul(UChar32 ch) {
    auto hst = static_cast<UHangulSyllableType>
        (u_getIntPropertyValue(ch, UCHAR_HANGUL_SYLLABLE_TYPE));

    return ( hst == U_HST_LEADING_JAMO
          || hst == U_HST_VOWEL_JAMO
          || hst == U_HST_TRAILING_JAMO
          || hst == U_HST_LV_SYLLABLE
          || hst == U_HST_LVT_SYLLABLE );
}
bool is_control(UChar32 ch) {
    if ( /*ch == 0x000D || ch == 0x000A ||*/ ch == 0x200C || ch == 0x200D) {
        return false;
    }
    if (u_getIntPropertyValue(ch, UCHAR_PREPENDED_CONCATENATION_MARK)) {
        return false;
    }
    auto gencat = static_cast<UCharCategory>(u_getIntPropertyValue(ch, UCHAR_GENERAL_CATEGORY));

    return gencat == U_LINE_SEPARATOR || gencat == U_PARAGRAPH_SEPARATOR
        || gencat == U_CONTROL_CHAR   || gencat == U_FORMAT_CHAR
        || ( gencat == U_UNASSIGNED
          && u_getIntPropertyValue(ch, UCHAR_DEFAULT_IGNORABLE_CODE_POINT));
}
void ensure_no_more_than_one_category(strf::outbuff& out, UChar32 ch)
{
    int is_zwj_                 = is_zwj(ch);
    int is_extend_              = is_extend(ch);
    int is_extended_picto_      = is_extended_picto(ch);
    int is_regional_indicator_  = is_regional_indicator(ch);
    int is_spacing_mark_        = is_spacing_mark(ch);
    int is_prepend_             = is_prepend(ch);
    int is_hangul_              = is_hangul(ch);
    int is_control_             = is_control(ch);

    int sum = is_zwj_ + is_extend_ + is_extended_picto_ + is_regional_indicator_
        + is_spacing_mark_ + is_prepend_ + is_hangul_ + is_control_;

    if (sum > 1) {
        strf::to(out).with(strf::mixedcase) ( "Character U+", strf::hex(ch).p(4), " is:");
        if (is_zwj_) strf::to(out) (" zwj");
        if (is_extend_) strf::to(out) (" extend");
        if (is_extended_picto_) strf::to(out) (" extended_picto");
        if (is_regional_indicator_) strf::to(out) (" regional_indicator");
        if (is_spacing_mark_) strf::to(out) (" spacing_mark");
        if (is_prepend_) strf::to(out) (" prepend");
        if (is_hangul_) strf::to(out) (" hangul");
        if (is_control_) strf::to(out) (" control");
        strf::to(out) ('\n');
    }
}

void ensure_no_more_than_one_category_per_char(strf::outbuff& out) {
    for (UChar32 ch = 0; ch <= 0x10FFFF; ++ch) {
        ensure_no_more_than_one_category(out, ch);
    }
}

bool is_double_width(UChar32 ch) {
    // from http://eel.is/c++draft/format.string.std#11
    return (0x1100 <= ch && ch <= 0x115F)
        || (0x2329 <= ch && ch <= 0x232A)
        || (0x2E80 <= ch && ch <= 0x303E)
        || (0x3040 <= ch && ch <= 0xA4CF)
        || (0xAC00 <= ch && ch <= 0xD7A3)
        || (0xF900 <= ch && ch <= 0xFAFF)
        || (0xFE10 <= ch && ch <= 0xFE19)
        || (0xFE30 <= ch && ch <= 0xFE6F)
        || (0xFF00 <= ch && ch <= 0xFF60)
        || (0xFFE0 <= ch && ch <= 0xFFE6)
        || (0x1F300 <= ch && ch <= 0x1F64F)
        || (0x1F900 <= ch && ch <= 0x1F9FF)
        || (0x20000 <= ch && ch <= 0x2FFFD)
        || (0x30000 <= ch && ch <= 0x3FFFD);
}

category category_widthout_width_mask(UChar32 ch) {
    int is_control = ::is_control(ch);
    int is_extend = ::is_extend(ch);

    if (is_control && is_extend) {
        return category::extend_and_control;
    }
    if (is_control) {
        return category::control;
    }
    if (is_extend) {
        return category::extend;
    }
    if (is_zwj(ch)) {
        return category::zwj;
    }
    if (is_spacing_mark(ch)) {
        return category::spacing_mark;
    }
    if (is_prepend(ch)) {
        return category::prepend;
    }
    if (is_extended_picto(ch)) {
        return category::extended_picto;
    }
    if (is_regional_indicator(ch)) {
        return category::regional_indicator;
    }
    auto hst = static_cast<UHangulSyllableType>
        (u_getIntPropertyValue(ch, UCHAR_HANGUL_SYLLABLE_TYPE));
    if (hst == U_HST_LEADING_JAMO) {
        return category::hangul_l;
    }
    if (hst == U_HST_VOWEL_JAMO) {
        return category::hangul_v;
    }
    if (hst == U_HST_TRAILING_JAMO) {
        return category::hangul_t;
    }
    if (hst == U_HST_LV_SYLLABLE || hst == U_HST_LVT_SYLLABLE) {
        return category::hangul_lv_or_lvt;
    }

    return category::other;
}

category category_of(UChar32 ch) {
    auto cat = category_widthout_width_mask(ch);
    int mask = is_double_width(ch) ? (int)category::dw_mask : 0;
    return mask | cat;
}

struct chars_range {
    category cat;
    UChar32 first;
    UChar32 last;
    int size() const { return last - first + 1; }
};

std::vector<chars_range> map_categories()
{
    std::vector<chars_range> v;
    category previous_cat = category::control;
    UChar32 first = 0;
    for (UChar32 ch = 0; ch <= 0x10FFFF; ++ch) {
        auto cat = category_of(ch);
        if (cat != previous_cat) {
            v.push_back(chars_range{previous_cat, first, ch - 1});
            previous_cat = cat;
            first = ch;
        }
    }
    v.push_back(chars_range{previous_cat, first, 0x10FFFF});
    return v;
}

void print_samples(strf::outbuff& dest, const chars_range* begin, const chars_range* end) {
    std::map<category, std::vector<unsigned>> m;
    for (auto it = begin; it < end; ++it) {
        m[it->cat].push_back(it->last);
    }
    auto print = strf::to(dest).with(strf::mixedcase);
    for (auto& p: m) {
        print( "    const char32_t samples_", to_str(p.first), "[] = {\n");
        for(std::size_t i = 0; i < p.second.size(); ) {
            std::size_t count = std::min((std::size_t)6, (p.second.size() - i));
            const auto* it = &p.second[i];
            print( "        "
                 , *strf::fmt_separated_range(it, it + count, ", ").hex().p(6),
                 ",\n" );
            i += count;
        }
        print("    };\n");
    }
}

void verify(strf::outbuff& dest, const chars_range* begin, const chars_range* end) {
    if (begin == end)
        return;

    auto print = strf::to(dest).with(strf::mixedcase);

    for (auto it = begin; it < end; ++it) {
        auto next = it + 1;
        const std::size_t index = it - begin;
        if (next != end) {
            if (it->last != (next->first - 1)) {
                print ( "range[", index, "].last == ", *strf::hex(it->last)
                        , "; range[", index + 1, "] == ",  *strf::hex(next->first), '\n' );
            }
            if (it->cat == next->cat) {
                print ( "range[", index, "].cat == range[", index + 1, "]\n" );
            }
        }
        for (auto ch = it->first; ch <= it->last; ++ch) {
            if (category_of(ch) != it->cat) {
                print ( "range[", index, "].cat == ", to_str(it->cat)
                      , " ; category_of(", *strf::hex(ch), ") == "
                      , to_str(category_of(ch)), '\n' );
            }
        }
    }
}

struct isolated_category {
    category cat;
    UChar32 ch;
};

struct chars_range_with_exceptions {
    category cat;
    UChar32 first;
    UChar32 last;
    std::vector<isolated_category> exceptions;
};

std::vector<chars_range_with_exceptions> compact
    ( const chars_range* begin
    , const chars_range* end )
{
    // std::vector<const chars_range*> isolated_elements;
    // isolated_elements.reserve(end - begin);
    assert(begin != end);
    std::vector<chars_range_with_exceptions> v;

    for (auto it = begin; it < end; ++it) {
        std::size_t index = it - begin;
        if (it->size() != 1 || it == begin || it == end - 1) {
            chars_range_with_exceptions r;
            r.cat = it->cat;
            r.first = it->first;
            r.last =  it->last;
            v.push_back(r);
            continue;
        }
        const auto previous_cat = v.empty() ? it[-1].cat : v.back().cat;
        const chars_range* it_next = nullptr;
        for (auto it2 = it; it2 < end; ++it2) {
            while(it2->size() == 1) {
                ++it2;
            }
            if (it2 == end) {
                break;
            }
            if (it2->cat != previous_cat)
                break;
            it_next = it2;
        };
        if (it_next != nullptr) {
            //assert(it_next->cat == it->cat);
            assert(it_next > it);
            chars_range_with_exceptions r;
            r.cat = previous_cat;
            r.first = it[-1].first;
            r.last =  it_next->last;
            do {
                if (it->cat != previous_cat) {
                    r.exceptions.push_back(isolated_category{it->cat, it->first});
                }
            } while (++it != it_next);
            if (v.back().first == r.first) {
                v.back() = r;
            } else {
                v.push_back(r);
            }
        } else {
            assert (v.back().last + 1 == it->first);
            v.back().last++;
            if (it->cat != v.back().cat) {
                v.back().exceptions.push_back(isolated_category{it->cat, it->first});
            }
        }
    }
    return v;
}


void verify
    ( strf::outbuff& dest
    , const chars_range_with_exceptions* begin
    , const chars_range_with_exceptions* end )
{
    auto print = strf::to(dest).with(strf::mixedcase);
    if (begin == end)
        return;

    for (auto it = begin; it < end; ++it) {
        auto next = it + 1;
        const std::size_t index = it - begin;
        if (next != end) {
            if (it->last != (next->first - 1)) {
                print ( "range[", index, "].last == ", *strf::hex(it->last)
                        , "; range[", index + 1, "] == ",  *strf::hex(next->first), '\n' );
            }
            if (it->cat == next->cat) {
                print ( "range[", index, "].cat == range[", index + 1, "]\n" );
            }
        }

        for (auto ch = it->first; ch <= it->last; ++ch) {
            auto x_it = std::find_if(it->exceptions.begin(), it->exceptions.end(),
                                     [ch](auto x){ return x.ch == ch; } );
            category cat = (x_it != it->exceptions.end() ? x_it->cat : it->cat);
            if (category_of(ch) != cat) {
                print ( " ; category_of(", *strf::hex(ch), ") == "
                      , to_str(category_of(ch)), '\n' );
            }
        }
    }
}

void print_categories_boundaries
    ( strf::outbuff& dest
    , const chars_range* begin
    , const chars_range* end )
{
    auto print = strf::to(dest).with(strf::mixedcase);
    for (auto it = begin; it != end; ++it) {
        print('[');
        if (it->size() == 1) {
            print(strf::hex(it->first) ^ 16);
        } else {
            print(strf::hex(it->first)>6, " .. ", strf::hex(it->last)>6);
        }
        print("] ", to_str(it->cat), '\n');
    }
}

void print_categories_boundaries
    ( strf::outbuff& dest
    , const chars_range_with_exceptions* begin
    , const chars_range_with_exceptions* end )
{
    auto print = strf::to(dest).with(strf::mixedcase);
    for (auto it = begin; it != end; ++it) {
        print(strf::hex(it->first).p(6), " .. ", strf::hex(it->last).p(6)
             , "  ", to_str(it->cat), '\n');
        for (auto&& x: it->exceptions) {
            print("          ", strf::hex(x.ch).p(6), "  ", to_str(x.cat), '\n');
        }
    }
}

constexpr unsigned line_length = 80;

void print_exception_goto
    ( strf::outbuff& dest
    , int indent_level
    , category parent_cat
    , category cat
    , UChar32 ch
    , bool ch_double_width_printed )
{
    auto print = strf::to(dest).with(strf::mixedcase);
    bool shall_print_ch_double_width = ! ch_double_width_printed && is_double_width(cat);
    bool shall_print_goto = ( remove_dw_flag(cat) != remove_dw_flag(parent_cat)
                           || ! shall_print_ch_double_width );
    //assert(shall_print_ch_double_width || shall_print_goto);

    if (shall_print_ch_double_width && shall_print_goto) {
        print(" {\n");
    } else {
        print('\n');
    }
    const auto sub_indentation = strf::multi(' ', (1 + indent_level)* indentation_factor);
    if (shall_print_ch_double_width)
        print(sub_indentation, "ch_width = 2;\n");
    if (shall_print_goto)
        print(sub_indentation, "goto handle_", to_str(remove_dw_flag(cat)), ";\n");
    if (shall_print_ch_double_width && shall_print_goto) {
        print(strf::multi(' ', indent_level* indentation_factor), "}\n");
    }
}


bool all_double_width
    ( const chars_range_with_exceptions* begin
    , const chars_range_with_exceptions* end )
{
    for (auto it = begin; it != end; ++it) {
        if (! is_double_width(it->cat)) {
            return false;
        }
        for (auto&& x: it->exceptions) {
            if (! is_double_width(x.cat)) {
                return false;
            }
        }
    }
    return true;
}


void print_branches
    ( strf::outbuff& dest
    , int indent_level
    , const chars_range_with_exceptions* begin
    , const chars_range_with_exceptions* end
    , bool ch_double_width_printed = false )
{
    auto print = strf::to(dest).with(strf::mixedcase);
    const auto indentation = strf::multi(' ', indent_level * indentation_factor);
    if (begin != end) {
        if ( ! ch_double_width_printed) {
            if (all_double_width(begin, end)) {
                print( indentation, "ch_width = 2;\n");
                ch_double_width_printed = true;
            }
        }
        auto count = end - begin;
        if (count == 1) {
            for (auto& x: begin->exceptions) {
                print( indentation, "if (ch == ", *strf::hex(x.ch).p(4), ")");
                print_exception_goto(dest, indent_level, begin->cat, x.cat, x.ch, ch_double_width_printed);
            }
            if (!ch_double_width_printed && is_double_width(begin->cat)) {
                print( indentation, "ch_width = 2;\n");
            }
            print(indentation, "goto handle_", to_str(remove_dw_flag(begin->cat)), ";\n");
        } else {
            auto half_count = count / 2;
            auto& boundary = begin[half_count - 1];
            print(strf::join_left(62)
                      ( indentation, "if (ch <= ", *strf::hex(boundary.last), ") {")
                 , "// ", strf::hex(begin->first).p(4) > 5
                 , " .." , strf::hex(boundary.last).p(4) > 6, '\n');
            print_branches(dest, indent_level + 1, begin, begin + half_count, ch_double_width_printed);
            print(strf::join_left(62) ( indentation, "} else { ")
                 , "// ", strf::hex(boundary.last + 1).p(4) > 5
                 , " ..", strf::hex((end - 1)->last).p(4) > 6, '\n');
            print_branches(dest, indent_level + 1, begin + half_count, end, ch_double_width_printed);
            print(indentation, "}\n");
        }
    }
}


int main()
{
    strf::narrow_cfile_writer<char, 1000> out(stdout);

    //ensure_no_more_than_one_category_per_char(out);
    auto v = map_categories();
    //print_samples(out, &v[0], &*v.end());
    //verify(out, &v[0], &*v.end());
    auto c = compact(&v[0], &*v.end());
    verify(out, &c[0], &*c.end());

    // print_categories_boundaries(out, &c[0], &*c.end());
    print_branches(out, 1, &c[2], &*c.end());
    // v0::print_branches(out, 0, &v[0], &*v.end());
    out.finish();
    return 0;
}
