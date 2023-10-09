#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#include <vector>
#include <memory>
#include <iostream>

void debug() {
    const char* filePath = "debug.log";
    FILE* file = fopen(filePath, "a+");
    if (file == NULL) {
        perror("failed to open file");
    }
    fprintf(file, "1");
    fclose(file);
    return;
}

extern "C" size_t LLVMFuzzerMutate(uint8_t *data, size_t size, size_t max_size);

size_t random_index(size_t size) {
    return rand() % size;
};

// random del content inside a pair of brackets(no matter inside < .. > or > .. <)
size_t mutate_del_bracket(uint8_t *data, size_t size, size_t max_size)  {
#ifdef DEBUG
    std::cout << "mutate_del_bracket" << std::endl;
#endif
    std::vector<size_t> lbracket_indices = std::vector<size_t>();
    std::vector<size_t> rbracket_indices = std::vector<size_t>();
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '<') {
            lbracket_indices.push_back(i);
        } else if (data[i] == '>') {
            rbracket_indices.push_back(i);
        }
    }
    if (lbracket_indices.size() == 0 || rbracket_indices.size() == 0) {
        return LLVMFuzzerMutate(data, size, max_size);
    }
    size_t lbracket_index = lbracket_indices[random_index(lbracket_indices.size())];
    size_t rbracket_index = rbracket_indices[random_index(rbracket_indices.size())];
    size_t new_size = size - std::abs(static_cast<long long>(lbracket_index) - static_cast<long long>(rbracket_index)) - 1;
    size_t l = std::min(lbracket_index, rbracket_index);
    size_t r = std::max(lbracket_index, rbracket_index);
    memmove(data, data, l);
    memmove(data + l, data + r + 1, size - r - 1);
    return LLVMFuzzerMutate(data, new_size, max_size);
}

size_t mutate_del_slash(uint8_t *data, size_t size, size_t max_size)  {
#ifdef DEBUG
    std::cout << "mutate_del_slash" << std::endl;
#endif
    std::vector<size_t> slash_indices = std::vector<size_t>();
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '/') {
            slash_indices.push_back(i);
        }
    }
    if (slash_indices.size() == 0) {
        return size;
    }
    size_t slash_index = slash_indices[random_index(slash_indices.size())];
    size_t new_size = size - 1;
    memmove(data + slash_index, data + slash_index + 1, size - slash_index - 1);
    return LLVMFuzzerMutate(data, new_size, max_size);
}

// random choose a brace to flip
size_t mutate_flip_bracket(uint8_t *data, size_t size, size_t max_size)  {
#ifdef DEBUG
    std::cout << "mutate_flip_bracket" << std::endl;
#endif
    std::vector<size_t> bracket_indices = std::vector<size_t>();
    for (size_t i = 0; i < size; i++) {
        if (data[i] == '<' || data[i] == '>') {
            bracket_indices.push_back(i);
        }
    }
    if (bracket_indices.size() == 0) {
        return size;
    }
    size_t brace_index = bracket_indices[random_index(bracket_indices.size())];
    data[brace_index] = data[brace_index] == '<' ? '>' : '<';
    return LLVMFuzzerMutate(data, size, max_size);
}

size_t mutate_insert(uint8_t *data, size_t size, size_t max_size)  {
#ifdef DEBUG
    std::cout << "mutate_insert" << std::endl;
#endif
    if (max_size < size + 2) {
        return size;
    }
    size_t left = random_index(size);
    memmove(data + left + 1, data + left, size - left);
    data[left] = '<';
    size_t right = random_index(size + 1);
    memmove(data + right + 1, data + right, size - right);
    data[right] = '>';
    return LLVMFuzzerMutate(data, size + 2, max_size);
}


extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *data, size_t size, size_t max_size, unsigned int seed) {
#ifdef DEBUG
    debug();
#endif
    const char* xmlString = 
    "<person id=\"1\">"
        "<name>John Doe</name>"
        "<age>30</age>"
        "<email>john.doe@example.com</email>"
    "</person>";

    if (size < sizeof(xmlString) && max_size >= sizeof(xmlString)) {
        memmove(data, xmlString, sizeof(xmlString) + 1);
        return sizeof(xmlString);
    }
    srand(seed);
    switch (rand() % 4) {
        case 0:
            return mutate_del_bracket(data, size, max_size);
        case 1:
            return mutate_del_slash(data, size, max_size);
        case 2:
            return mutate_flip_bracket(data, size, max_size);
        case 3:
            return mutate_insert(data, size, max_size);
    }
    return size;
}
