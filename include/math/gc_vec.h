#ifndef GC_MATH_VEC_H_
#define GC_MATH_VEC_H_

#include <stdint.h>

typedef struct {
  float x;
  float y;
} Vec2f;

Vec2f vec2f_add(Vec2f a, Vec2f b);
Vec2f vec2f_sub(Vec2f a, Vec2f b);
Vec2f vec2f_mul(Vec2f a, Vec2f b);

typedef struct {
  int32_t x;
  int32_t y;
} Vec2i;

Vec2i vec2i_add(Vec2i a, Vec2i b);
Vec2i vec2i_sub(Vec2i a, Vec2i b);
Vec2i vec2i_mul(Vec2i a, Vec2i b);

typedef struct {
  float x;
  float y;
  float z;
} Vec3f;

Vec3f vec3f_add(Vec3f a, Vec3f b);
Vec3f vec3f_sub(Vec3f a, Vec3f b);
Vec3f vec3f_mul(Vec3f a, Vec3f b);

typedef struct {
  int32_t x;
  int32_t y;
  int32_t z;
} Vec3i;

Vec3i vec3i_add(Vec3i a, Vec3i b);
Vec3i vec3i_sub(Vec3i a, Vec3i b);
Vec3i vec3i_mul(Vec3i a, Vec3i b);

#endif // GC_MATH_VEC_H_