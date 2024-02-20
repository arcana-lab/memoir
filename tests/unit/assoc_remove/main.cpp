#include <cstdio>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define KEY0 (uint64_t)1
#define KEY1 (uint64_t)2
#define KEY2 (uint64_t)3

#define VAL0 10
#define VAL1 20
#define VAL2 30

#define EXPECTED0 true
#define EXPECTED1 false
#define EXPECTED2 true

int main() {
  printf("Initializing map\n");

  auto map = memoir_allocate_assoc_array(memoir_u64_t, memoir_u64_t);

  memoir_assoc_insert(map, KEY0);
  memoir_assoc_write(u64, VAL0, map, KEY0);
  memoir_assoc_insert(map, KEY1);
  memoir_assoc_write(u64, VAL1, map, KEY1);
  memoir_assoc_insert(map, KEY2);
  memoir_assoc_write(u64, VAL2, map, KEY2);

  printf("Removing from map\n");

  memoir_assoc_remove(map, KEY1);

  printf("Reading map\n");

  auto has0 = memoir_assoc_has(map, KEY0);
  auto has1 = memoir_assoc_has(map, KEY1);
  auto has2 = memoir_assoc_has(map, KEY2);

  printf(" Result:\n");
  printf("  %d -> %s\n", KEY0, has0 ? "true" : "false");
  printf("  %d -> %s\n", KEY1, has1 ? "true" : "false");
  printf("  %d -> %s\n", KEY2, has2 ? "true" : "false");

  printf(" Result:\n");
  printf("  %d -> %s\n", KEY0, EXPECTED0 ? "true" : "false");
  printf("  %d -> %s\n", KEY1, EXPECTED1 ? "true" : "false");
  printf("  %d -> %s\n", KEY2, EXPECTED2 ? "true" : "false");
}
