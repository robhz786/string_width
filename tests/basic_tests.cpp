#include <string_width.hpp>

int main() {

    // to-do: Copy and adapt tests from github.com/robhz786/strf
    // For now, just testing the bare basic:
    {
        auto w = string_width::str_width(1000, "abcd", 4);
        assert(w == 4);
    }
    {
        auto w = string_width::str_width(3, "abcd", 4);
        assert(w == 3);
    }   
    {
        auto r = string_width::str_width_and_pos(1000, "abcd", 4);
        assert(r.width == 4);
        assert(r.pos == 4);
    }
    {
        auto r = string_width::str_width_and_pos(3, "abcd", 4);
        assert(r.width == 3);
        assert(r.pos == 3);
    }
    
    return 0;
}
    
