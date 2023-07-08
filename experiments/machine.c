// x86/x64 Runtime Code Generation Demonstration (Linux/macOS/Windows)
// Copyright (C) 2017 Jeffrey L. Overbey.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
// WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
// SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
// WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
// OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
// CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#include <stddef.h>   // size_t
#include <stdint.h>   // uint8_t, uint32_t
#include <stdio.h>    // printf
#include <string.h>   // memcpy
#include <sys/mman.h> // mmap, mprotect, munmap, MAP_FAILURE

// Machine code for "mov eax, 12345678h" followed by "ret"
uint8_t machine_code[] = {0x20, 0x00, 0x80, 0xd2};

int main(int argc, char **argv) {
  // Allocate a new page of memory, setting its protections to read+write
  void *mem = mmap(NULL, sizeof(machine_code), PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (mem == MAP_FAILED) {
    perror("mmap");
    return 1;
  }

  // Write the machine code into the newly allocated page
  memcpy(mem, machine_code, sizeof(machine_code));

  // Change the page protections to read+execute
  if (mprotect(mem, sizeof(machine_code), PROT_READ | PROT_EXEC) == -1) {
    perror("mprotect");
    return 2;
  }

  // Point a function pointer at the newly allocated page, then call it
  uint32_t (*fn)() = (uint32_t(*)())mem;
  uint32_t result = fn();
  printf("result = 0x%x\n", result);

  // Free the memory
  if (munmap(mem, sizeof(machine_code)) == -1) {
    perror("munmap");
    return 3;
  }

  return 0;
}
