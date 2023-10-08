#include <cstddef>
#include <cstdint>
#include <cstdio>

extern "C" size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed) {
	fprintf(stderr, "Using libFuzzer's mutator\n");
	return LLVMFuzzerMutate(Data, Size, MaxSize);
}
