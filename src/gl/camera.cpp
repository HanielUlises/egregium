#include "gl/camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>

namespace gl {

void Camera::orbit(float dYawDeg, float dPitchDeg) {
    yawDeg_ += dYawDeg;
    pitchDeg_ = std::clamp(pitchDeg_ + dPitchDeg, -89.0f, 89.0f);
}

void Camera::zoom(float scrollDelta) {
    distance_ *= std::pow(0.9f, scrollDelta);
    distance_ = std::clamp(distance_, 0.3f, 80.0f);
}

void Camera::pan(float dxNdc, float dyNdc) {
    glm::vec3 eye = eyePosition();
    glm::vec3 forward = glm::normalize(target_ - eye);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));
    float speed = distance_ * 0.9f;
    target_ += (-dxNdc * right + dyNdc * up) * speed;
}

void Camera::setView(float yawDeg, float pitchDeg, float distance) {
    yawDeg_ = yawDeg;
    pitchDeg_ = std::clamp(pitchDeg, -89.0f, 89.0f);
    distance_ = std::clamp(distance, 0.3f, 80.0f);
}

void Camera::reset() {
    yawDeg_ = -50.0f;
    pitchDeg_ = 22.0f;
    distance_ = 6.0f;
    target_ = glm::vec3(0.0f);
}

glm::vec3 Camera::eyePosition() const {
    float yaw = glm::radians(yawDeg_);
    float pitch = glm::radians(pitchDeg_);
    glm::vec3 offset(std::cos(pitch) * std::sin(yaw), std::sin(pitch), std::cos(pitch) * std::cos(yaw));
    return target_ + offset * distance_;
}

glm::mat4 Camera::viewMatrix() const { return glm::lookAt(eyePosition(), target_, glm::vec3(0, 1, 0)); }

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fovDeg_), aspect, nearZ_, farZ_);
}

} // namespace gl
