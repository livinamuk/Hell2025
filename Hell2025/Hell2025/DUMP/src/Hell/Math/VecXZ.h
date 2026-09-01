#pragma once

namespace Hell {
    struct vecXZ {
        float x = 0.0f;
        float z = 0.0f;

        vecXZ() : x(0.0f), z(0.0f) {}
        vecXZ(float x, float z) : x(x), z(z) {}

        bool operator==(const vecXZ& other) const {
            return x == other.x && z == other.z;
        }

        bool operator!=(const vecXZ& other) const {
            return !(*this == other);
        }

        bool operator<(const vecXZ& other) const {
            return (x < other.x) || (x == other.x && z < other.z);
        }

        bool operator>(const vecXZ& other) const {
            return other < *this;
        }

        bool operator<=(const vecXZ& other) const {
            return !(other < *this);
        }

        bool operator>=(const vecXZ& other) const {
            return !(*this < other);
        }
    };

    struct ivecXZ {
        int x = 0;
        int z = 0;

        ivecXZ() : x(0), z(0) {}
        ivecXZ(int x, int z) : x(x), z(z) {}

        bool operator==(const ivecXZ& other) const {
            return x == other.x && z == other.z;
        }

        bool operator!=(const ivecXZ& other) const {
            return !(*this == other);
        }

        bool operator<(const ivecXZ& other) const {
            return (x < other.x) || (x == other.x && z < other.z);
        }

        bool operator>(const ivecXZ& other) const {
            return other < *this;
        }

        bool operator<=(const ivecXZ& other) const {
            return !(other < *this);
        }

        bool operator>=(const ivecXZ& other) const {
            return !(*this < other);
        }
    };
}