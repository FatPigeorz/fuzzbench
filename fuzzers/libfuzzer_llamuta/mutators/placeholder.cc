#include <cstddef>
#include <cstdint>
#include <cstdio>

void debug() {
	const char *filePath = "debug.log";
	FILE *file = fopen(filePath, "a+");
	if (file == nullptr) {
		perror("Error opening file\n");
		return;
	}
    	fprintf(file, "1");
    	fclose(file);
	return;
}

// Forward-declare the customized mutator callback.
// Return the new size of mutated data.
extern "C" size_t LLVMFuzzerMutate(uint8_t *Data, size_t Size, size_t MaxSize);

extern "C" size_t LLVMFuzzerCustomMutator(uint8_t *Data, size_t Size, size_t MaxSize, unsigned int Seed) {
	debug();
	return LLVMFuzzerMutate(Data, Size, MaxSize);
}
