// Weak implementation of C++ exception personality routine for no-exception mode.
// This satisfies linker references when the C++ runtime is not available,
// but gets overridden by the strong symbol from libc++ when linking in C++ mode.

extern "C" int __attribute__((weak)) __gxx_personality_v0;
int __attribute__((weak)) __gxx_personality_v0 = 0;
