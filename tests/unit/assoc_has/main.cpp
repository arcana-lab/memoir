#include <cstdio>

#include "cmemoir/cmemoir.h"

using namespace memoir;

#define KEY0 1
#define KEY1 2
#define KEY2 3
#define INVALID_KEY 1000

#define VAL0 10
#define VAL1 20
#define VAL2 30

#define EXPECTED0 true
#define EXPECTED1 true
#define EXPECTED2 true
#define EXPECTED3 false

int main() {
  printf("Initializing map\n");

  auto map = memoir_allocate_assoc_array(memoir_u64_t, memoir_u64_t);

  memoir_assoc_write(u64, VAL0, map, KEY0);
  memoir_assoc_write(u64, VAL1, map, KEY1);
  memoir_assoc_write(u64, VAL2, map, KEY2);

  printf("Reading map\n");

  auto has0 = memoir_assoc_has(map, KEY0);
  auto has1 = memoir_assoc_has(map, KEY1);
  auto has2 = memoir_assoc_has(map, KEY2);
  auto has3 = memoir_assoc_has(map, INVALID_KEY);

  printf(" Result:\n");
  printf("  %d -> %s\n", KEY0, has0 ? "true" : "false");
  printf("  %d -> %s\n", KEY1, has1 ? "true" : "false");
  printf("  %d -> %s\n", KEY2, has2 ? "true" : "false");
  printf("  %d -> %s\n", INVALID_KEY, has3 ? "true" : "false");

  printf(" Result:\n");
  printf("  %d -> %s\n", KEY0, EXPECTED0 ? "true" : "false");
  printf("  %d -> %s\n", KEY1, EXPECTED1 ? "true" : "false");
  printf("  %d -> %s\n", KEY2, EXPECTED2 ? "true" : "false");
  printf("  %d -> %s\n", INVALID_KEY, EXPECTED3 ? "true" : "false");
}
