#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <arpa/inet.h>
#include <stdint.h>
#include <string.h>

static inline uint32_t float_to_net(float f) {
  uint32_t val;
  memcpy(&val, &f, sizeof(float));
  return htonl(val);
}

static inline float net_to_float(uint32_t v) {
  v = ntohl(v);
  float f;
  memcpy(&f, &v, sizeof(float));
  return f;
}

#endif // SERIALIZE_H
