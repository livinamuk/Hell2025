#include "Util.h"
#include "HellConstants.h"

namespace Util {

    const AnimatedNode* FindAnimatedNode(Animation* animation, const char* NodeName) {
        for (unsigned int i = 0; i < animation->m_animatedNodes.size(); i++) {
            const AnimatedNode* animatedNode = &animation->m_animatedNodes[i];

            if (Util::StrCmp(animatedNode->m_nodeName, NodeName)) {
                return animatedNode;
            }
        }
        return nullptr;
    }

    int FindAnimatedNodeIndex(float AnimationTime, const AnimatedNode* animatedNode) {
        if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
            return -1;
        }

        // bail if current animation time is earlier than the this nodes first keyframe time
        if (AnimationTime < animatedNode->m_nodeKeys[0].timeStamp)
            return -1; // you WERE returning -1 here

        for (unsigned int i = 1; i < animatedNode->m_nodeKeys.size(); i++) {
            if (AnimationTime < animatedNode->m_nodeKeys[i].timeStamp)
                return i - 1;
        }
        return (int)animatedNode->m_nodeKeys.size() - 1;
    }

    void CalcInterpolatedPosition(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
            Out = glm::vec3(0.0f);
            return;
        }

        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].positon;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].positon;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        if (DeltaTime == 0.0f) {
            Out = animatedNode->m_nodeKeys[Index].positon;
            return;
        }
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        glm::vec3 start = animatedNode->m_nodeKeys[Index].positon;
        glm::vec3 end = animatedNode->m_nodeKeys[NextIndex].positon;
        glm::vec3 delta = end - start;
        Out = start + Factor * delta;
    }

    void CalcInterpolatedScale(glm::vec3& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
            Out = glm::vec3(1.0f);
            return;
        }

        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].scale;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].scale;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        if (DeltaTime == 0.0f) {
            Out = animatedNode->m_nodeKeys[Index].scale;
            return;
        }
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        glm::vec3 start = animatedNode->m_nodeKeys[Index].scale;
        glm::vec3 end = animatedNode->m_nodeKeys[NextIndex].scale;
        glm::vec3 delta = end - start;
        Out = start + Factor * delta;
    }

    void CalcInterpolatedRotation(glm::quat& Out, float AnimationTime, const AnimatedNode* animatedNode) {
        if (!animatedNode || animatedNode->m_nodeKeys.empty()) {
            Out = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            return;
        }

        int Index = FindAnimatedNodeIndex(AnimationTime, animatedNode);
        int NextIndex = (Index + 1);

        // Is next frame out of range?
        if (NextIndex == animatedNode->m_nodeKeys.size()) {
            Out = animatedNode->m_nodeKeys[Index].rotation;
            return;
        }

        // Nothing to report
        if (Index == -1 || animatedNode->m_nodeKeys.size() == 1) {
            Out = animatedNode->m_nodeKeys[0].rotation;
            return;
        }
        float DeltaTime = animatedNode->m_nodeKeys[NextIndex].timeStamp - animatedNode->m_nodeKeys[Index].timeStamp;
        if (DeltaTime == 0.0f) {
            Out = animatedNode->m_nodeKeys[Index].rotation;
            return;
        }
        float Factor = (AnimationTime - animatedNode->m_nodeKeys[Index].timeStamp) / DeltaTime;

        const glm::quat& StartRotationQ = animatedNode->m_nodeKeys[Index].rotation;
        const glm::quat& EndRotationQ = animatedNode->m_nodeKeys[NextIndex].rotation;

        Util::InterpolateQuaternion(Out, StartRotationQ, EndRotationQ, Factor);
        Out = glm::normalize(Out);
    }

    glm::mat4 Mat4InitScaleTransform(float ScaleX, float ScaleY, float ScaleZ) {
        return glm::scale(glm::mat4(1.0), glm::vec3(ScaleX, ScaleY, ScaleZ));
    }

    glm::mat4 Mat4InitTranslationTransform(float x, float y, float z) {
        return  glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    }

    glm::mat4 Mat4InitRotateTransform(float RotateX, float RotateY, float RotateZ) {
        glm::mat4 rx = glm::mat4(1);
        glm::mat4 ry = glm::mat4(1);
        glm::mat4 rz = glm::mat4(1);

        const float x = glm::radians(RotateX);
        const float y = glm::radians(RotateY);
        const float z = glm::radians(RotateZ);

        rx[0][0] = 1.0f; rx[0][1] = 0.0f; rx[0][2] = 0.0f; rx[0][3] = 0.0f;
        rx[1][0] = 0.0f; rx[1][1] = cosf(x); rx[1][2] = -sinf(x); rx[1][3] = 0.0f;
        rx[2][0] = 0.0f; rx[2][1] = sinf(x); rx[2][2] = cosf(x); rx[2][3] = 0.0f;
        rx[3][0] = 0.0f; rx[3][1] = 0.0f; rx[3][2] = 0.0f; rx[3][3] = 1.0f;

        ry[0][0] = cosf(y); ry[0][1] = 0.0f; ry[0][2] = -sinf(y); ry[0][3] = 0.0f;
        ry[1][0] = 0.0f; ry[1][1] = 1.0f; ry[1][2] = 0.0f; ry[1][3] = 0.0f;
        ry[2][0] = sinf(y); ry[2][1] = 0.0f; ry[2][2] = cosf(y); ry[2][3] = 0.0f;
        ry[3][0] = 0.0f; ry[3][1] = 0.0f; ry[3][2] = 0.0f; ry[3][3] = 1.0f;

        rz[0][0] = cosf(z); rz[0][1] = -sinf(z); rz[0][2] = 0.0f; rz[0][3] = 0.0f;
        rz[1][0] = sinf(z); rz[1][1] = cosf(z); rz[1][2] = 0.0f; rz[1][3] = 0.0f;
        rz[2][0] = 0.0f; rz[2][1] = 0.0f; rz[2][2] = 1.0f; rz[2][3] = 0.0f;
        rz[3][0] = 0.0f; rz[3][1] = 0.0f; rz[3][2] = 0.0f; rz[3][3] = 1.0f;

        return rz * ry * rx;
    }

    float SmoothStep(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    }

    float SmoothStepReverse(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return 1.0f - (t * t * (3.0f - 2.0f * t));
    }

    float SteepSlowDownCurve(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return t * t * t;
    }

    float EaseOut(float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        return 1.0f - (1.0f - t) * (1.0f - t);
    }

    float Sanitize(float value) {
        const float threshold = 0.002f;
        if (std::abs(value) < threshold) return 0.0f;
        if (std::abs(value - 1.0f) < threshold) return 1.0f;
        if (std::abs(value + 1.0f) < threshold) return -1.0f;
        return value;

        //// Catch values near 0
        //if (std::abs(value) < 1e-5f) return 0.0f;
        //
        //// Catch values near 1.0
        //if (std::abs(value - 1.0f) < 1e-5f) return 1.0f;
        //
        //// Catch values near -1.0
        //if (std::abs(value + 1.0f) < 1e-5f) return -1.0f;
        //
        //return value;
    }

    glm::vec3 SanitizeVec3(const glm::vec3& v) {
        return glm::vec3(Sanitize(v.x), Sanitize(v.y), Sanitize(v.z));
    }

    glm::quat SanitizeQuat(const glm::quat& q) {
        glm::quat result = q;
        result.x = Sanitize(q.x);
        result.y = Sanitize(q.y);
        result.z = Sanitize(q.z);
        result.w = Sanitize(q.w);
        return glm::normalize(result);
    }

    void SanitizeMat4(glm::mat4& m) {
        const float threshold = 1e-5f;

        for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 4; ++row) {
                float& val = m[col][row];
                float absVal = std::abs(val);

                // Snap near 0
                if (absVal < threshold) {
                    val = 0.0f;
                }
                // Snap near 1
                else if (std::abs(val - 1.0f) < threshold) {
                    val = 1.0f;
                }
                // Snap near -1
                else if (std::abs(val + 1.0f) < threshold) {
                    val = -1.0f;
                }
            }
        }
    }

    glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
        glm::mat4 to;
        to[0][0] = Sanitize(from.a1); to[1][0] = Sanitize(from.a2); to[2][0] = Sanitize(from.a3); to[3][0] = Sanitize(from.a4);
        to[0][1] = Sanitize(from.b1); to[1][1] = Sanitize(from.b2); to[2][1] = Sanitize(from.b3); to[3][1] = Sanitize(from.b4);
        to[0][2] = Sanitize(from.c1); to[1][2] = Sanitize(from.c2); to[2][2] = Sanitize(from.c3); to[3][2] = Sanitize(from.c4);
        to[0][3] = Sanitize(from.d1); to[1][3] = Sanitize(from.d2); to[2][3] = Sanitize(from.d3); to[3][3] = Sanitize(from.d4);
        return to;
    }

    glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3& from) {
        glm::mat4 to;
        to[0][0] = Sanitize(from.a1); to[1][0] = Sanitize(from.a2); to[2][0] = Sanitize(from.a3); to[3][0] = 0.0f;
        to[0][1] = Sanitize(from.b1); to[1][1] = Sanitize(from.b2); to[2][1] = Sanitize(from.b3); to[3][1] = 0.0f;
        to[0][2] = Sanitize(from.c1); to[1][2] = Sanitize(from.c2); to[2][2] = Sanitize(from.c3); to[3][2] = 0.0f;
        to[0][3] = 0.0f; to[1][3] = 0.0f; to[2][3] = 0.0f; to[3][3] = 1.0f;
        return to;
    }

    //glm::mat4 aiMatrix4x4ToGlm(const aiMatrix4x4& from) {
    //    glm::mat4 to;
    //    //the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
    //    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    //    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    //    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    //    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    //    return to;
    //}
    //
    //glm::mat4 aiMatrix3x3ToGlm(const aiMatrix3x3& from) {
    //    glm::mat4 to;
    //    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = 0.0;
    //    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = 0.0;
    //    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = 0.0;
    //    to[0][3] = 0.0; to[1][3] = 0.0; to[2][3] = 0.0; to[3][3] = 1.0;
    //    return to;
    //}
}
