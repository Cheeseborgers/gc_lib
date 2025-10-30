#include "gc_vec.h"

Vec2f vec2f_add(Vec2f a, Vec2f b) {
  Vec2f ret = { .x = a.x + b.x, .y = a.y + b.y };;
  return ret;
}
Vec2f vec2f_sub(Vec2f a, Vec2f b) {
  Vec2f ret = { .x = a.x - b.x, .y = a.y - b.y };;
  return ret;
}
Vec2f vec2f_mul(Vec2f a, Vec2f b) {
  Vec2f ret = { .x = a.x * b.x, .y = a.y * b.y };;
  return ret;
}

Vec2i vec2i_add(Vec2i a, Vec2i b) {
  Vec2i ret = { .x = a.x + b.x, .y = a.y + b.y };
  return ret;
}
Vec2i vec2i_sub(Vec2i a, Vec2i b) {
  Vec2i ret = { .x = a.x - b.x, .y = a.y - b.y };;
  return ret;
}
Vec2i vec2i_mul(Vec2i a, Vec2i b) {
  Vec2i ret = { .x = a.x * b.x, .y = a.y * b.y };;
  return ret;
}

Vec3f vec3f_add(Vec3f a, Vec3f b) {
  Vec3f ret = { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };;
  return ret;
}
Vec3f vec3f_sub(Vec3f a, Vec3f b) {
  Vec3f ret = { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };;
  return ret;
}
Vec3f vec3f_mul(Vec3f a, Vec3f b) {
  Vec3f ret = { .x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z };;
  return ret;
}

Vec3i vec3i_add(Vec3i a, Vec3i b) {
  Vec3i ret = { .x = a.x + b.x, .y = a.y + b.y, .z = a.z + b.z };;
  return ret;
}
Vec3i vec3i_sub(Vec3i a, Vec3i b) {
  Vec3i ret = { .x = a.x - b.x, .y = a.y - b.y, .z = a.z - b.z };;
  return ret;
}
Vec3i vec3i_mul(Vec3i a, Vec3i b) {
  Vec3i ret = { .x = a.x * b.x, .y = a.y * b.y, .z = a.z * b.z };;
  return ret;
}