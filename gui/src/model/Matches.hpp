#pragma once

#include <cstddef>
#include <vector>

struct Matches {
    std::vector<size_t> offsets;

    size_t len = 0;     
    size_t current = 0; 
};
