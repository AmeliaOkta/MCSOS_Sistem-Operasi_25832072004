#ifndef MCSOS_M11_LOADER_OPS_H
#define MCSOS_M11_LOADER_OPS_H
#include <stdint.h>

struct mcsos_user_loader_ops {
  int (*alloc_user_page)(uint64_t user_va, uint32_t flags);
  int (*copy_to_user_mapping)(uint64_t user_va, const void *src, uint64_t len);
  int (*zero_user_mapping)(uint64_t user_va, uint64_t len);
  void (*trace)(const char *msg);
};

#endif
