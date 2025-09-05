#include "MITSUDomoe/Camera.hpp"
#include <Eigen/Geometry>

namespace MITSU_Domoe {

namespace {
    Eigen::Matrix4f lookAt(const Eigen::Vector3f& position, const Eigen::Vector3f& target, const Eigen::Vector3f& up) {
        Eigen::Vector3f f = (target - position).normalized();
        Eigen::Vector3f s = f.cross(up).normalized();
        Eigen::Vector3f u = s.cross(f);

        Eigen::Matrix4f mat = Eigen::Matrix4f::Identity();
        mat(0, 0) = s.x(); mat(0, 1) = s.y(); mat(0, 2) = s.z(); mat(0, 3) = -s.dot(position);
        mat(1, 0) = u.x(); mat(1, 1) = u.y(); mat(1, 2) = u.z(); mat(1, 3) = -u.dot(position);
        mat(2, 0) = -f.x(); mat(2, 1) = -f.y(); mat(2, 2) = -f.z(); mat(2, 3) = f.dot(position);
        return mat;
    }

    Eigen::Matrix4d lookAt(const Eigen::Vector3d& position, const Eigen::Vector3d& target, const Eigen::Vector3d& up) {
        Eigen::Vector3d f = (target - position).normalized();
        Eigen::Vector3d s = f.cross(up).normalized();
        Eigen::Vector3d u = s.cross(f);

        Eigen::Matrix4d mat = Eigen::Matrix4d::Identity();
        mat(0, 0) = s.x(); mat(0, 1) = s.y(); mat(0, 2) = s.z(); mat(0, 3) = -s.dot(position);
        mat(1, 0) = u.x(); mat(1, 1) = u.y(); mat(1, 2) = u.z(); mat(1, 3) = -u.dot(position);
        mat(2, 0) = -f.x(); mat(2, 1) = -f.y(); mat(2, 2) = -f.z(); mat(2, 3) = f.dot(position);
        return mat;
    }
}


Camera::Camera(Eigen::Vector3f position, Eigen::Vector3f up, float yaw, float pitch) : Front(Eigen::Vector3f(0.0f, 0.0f, -1.0f)), MovementSpeed(SPEED), MouseSensitivity(SENSITIVITY), Zoom(ZOOM) {
    Position = position;
    WorldUp = up;
    Yaw = yaw;
    Pitch = pitch;
    updateCameraVectors();
}

Eigen::Matrix4f Camera::GetViewMatrix() {
    return lookAt(Position, Position + Front, Up);
}

Eigen::Matrix4d Camera::GetViewMatrixDouble() {
    Eigen::Vector3d pos_d = Position.cast<double>();
    Eigen::Vector3d front_d = Front.cast<double>();
    Eigen::Vector3d up_d = Up.cast<double>();
    return lookAt(pos_d, pos_d + front_d, up_d);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = MovementSpeed * deltaTime;
    if (direction == FORWARD)
        Position += Front * velocity;
    if (direction == BACKWARD)
        Position -= Front * velocity;
    if (direction == LEFT)
        Position -= Right * velocity;
    if (direction == RIGHT)
        Position += Right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= MouseSensitivity;
    yoffset *= MouseSensitivity;

    Yaw += xoffset;
    Pitch += yoffset;

    if (constrainPitch) {
        if (Pitch > 89.0f)
            Pitch = 89.0f;
        if (Pitch < -89.0f)
            Pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    Zoom -= (float)yoffset;
    if (Zoom < 1.0f)
        Zoom = 1.0f;
    if (Zoom > 45.0f)
        Zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    Eigen::Vector3f front;
    float yawRad = Yaw * M_PI / 180.0f;
    float pitchRad = Pitch * M_PI / 180.0f;
    front.x() = cos(yawRad) * cos(pitchRad);
    front.y() = sin(pitchRad);
    front.z() = sin(yawRad) * cos(pitchRad);
    Front = front.normalized();
    Right = Front.cross(WorldUp).normalized();
    Up = Right.cross(Front).normalized();
}

}
