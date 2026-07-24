#pragma once
#include <glm/glm.hpp>

namespace gl {

// Spherical orbit camera around a target point.
class Camera {
public:
    void orbit(float dYawDeg, float dPitchDeg);
    void pan(float dxNdc, float dyNdc);
    void zoom(float scrollDelta);
    void reset();

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;
    glm::vec3 eyePosition() const;
    glm::vec3 target() const { return target_; }
    float nearZ() const { return nearZ_; }
    float farZ() const { return farZ_; }

private:
    float yawDeg_ = -50.0f;
    float pitchDeg_ = 22.0f;
    float distance_ = 6.0f;
    glm::vec3 target_{0.0f, 0.0f, 0.0f};
    float fovDeg_ = 45.0f;
    float nearZ_ = 0.05f;
    float farZ_ = 200.0f;
};

} // namespace gl
