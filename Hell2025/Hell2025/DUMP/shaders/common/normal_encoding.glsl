float SignAintZero(float value) {
    return value >= 0.0 ? 1.0 : -1.0;
}

vec3 DecodeOct(vec2 encoded) {
    vec2 normalXZ = encoded * 2.0 - 1.0;

    vec3 normal;
    normal.x = normalXZ.x;
    normal.z = normalXZ.y;
    normal.y = 1.0 - abs(normal.x) - abs(normal.z);

    if (normal.y < 0.0) {
        vec2 foldedNormalXZ = normal.xz;

        normal.x = (1.0 - abs(foldedNormalXZ.y)) * SignAintZero(foldedNormalXZ.x);
        normal.z = (1.0 - abs(foldedNormalXZ.x)) * SignAintZero(foldedNormalXZ.y);
    }

    return normalize(normal);
}

vec2 EncodeOct(vec3 direction) {
    direction = normalize(direction);

    float denominator = abs(direction.x) + abs(direction.y) + abs(direction.z);
    vec2 encoded = direction.xz / denominator;

    if (direction.y <= 0.0) {
        encoded = vec2(
            (1.0 - abs(encoded.y)) * (encoded.x >= 0.0 ? 1.0 : -1.0),
            (1.0 - abs(encoded.x)) * (encoded.y >= 0.0 ? 1.0 : -1.0)
        );
    }

    return encoded * 0.5 + 0.5;
}
