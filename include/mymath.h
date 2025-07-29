
#include <math.h>

f32 Lerp(f32 a, f32 t, f32 b) {
    f32 result = (1.0f - t)*a + t*b;
    return result;
}

Vector2 V2Lerp(Vector2 v1, f32 t, Vector2 v2) {
    // This is another identical equation for doing a lerp 
    Vector2 result = {v1.x + t*(v2.x - v1.x), v1.y + t*(v2.y - v1.y)};
    return result;
}


f32 Dot(Vector2 a, Vector2 b) {
    f32 result = a.x*b.x + a.y*b.y;
    return result;
}

f32 Dot(Vector3 a, Vector3 b) {
    f32 result = a.x*b.x + a.y*b.y + a.z*b.z;
    return result;
}

f32 LengthSq(Vector2 a) {
    f32 result = Dot(a, a);
    return result;
}

f32 LengthSq(Vector3 a) {
    f32 result = Dot(a, a);
    return result;
}

f32 SquareRoot(f32 a) {
    f32 result = sqrtf(a);
    return result;
}

f32 Length(Vector2 a) {
    f32 result = SquareRoot(LengthSq(a));
    return result;
}

f32 Length(Vector3 a) {
    f32 result = SquareRoot(LengthSq(a));
    return result;
}

Vector2 VectorScale(Vector2 a, f32 scale) {
    Vector2 result = {a.x*scale, a.y*scale};
    return result;
}

Vector3 VectorScale(Vector3 a, f32 scale) {
    Vector3 result = {a.x*scale, a.y*scale, a.z*scale};
    return result;
}

Vector2 VectorNorm(Vector2 a) {
    f32 inverse_length = 1.0f / Length(a);
    Vector2 result = {a.x*inverse_length, a.y*inverse_length};
    return result;
}

Vector3 VectorNorm(Vector3 a) {
    f32 inverse_length = 1.0f / Length(a);
    Vector3 result = {a.x*inverse_length, a.y*inverse_length, a.z*inverse_length};
    return result;
}

Vector2 VectorAdd(Vector2 a, Vector2 b) {
    Vector2 result = {(a.x + b.x), (a.y + b.y)};
    return result;
}

Vector3 VectorAdd(Vector3 a, Vector3 b) {
    Vector3 result = {(a.x + b.x), (a.y + b.y), (a.z + b.z)};
    return result;
}

Vector2 VectorSub(Vector2 a, Vector2 b) {
    Vector2 result = {(a.x - b.x), (a.y - b.y)};
    return result;
}

Vector3 VectorSub(Vector3 a, Vector3 b) {
    Vector3 result = {(a.x - b.x), (a.y - b.y), (a.z - b.z)};
    return result;
}
