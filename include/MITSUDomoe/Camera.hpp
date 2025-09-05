#pragma once

#include <Eigen/Dense>

namespace MITSU_Domoe {

enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

const float YAW         = -90.0f;
const float PITCH       =  0.0f;
const float SPEED       =  2.5f;
const float SENSITIVITY =  0.1f;
const float ZOOM        =  45.0f;

class Camera {
public:
    Eigen::Vector3f Position;
    Eigen::Vector3f Front;
    Eigen::Vector3f Up;
    Eigen::Vector3f Right;
    Eigen::Vector3f WorldUp;

    float Yaw;
    float Pitch;

    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    Camera(Eigen::Vector3f position = Eigen::Vector3f(0.0f, 0.0f, 0.0f), Eigen::Vector3f up = Eigen::Vector3f(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH);

    Eigen::Matrix4f GetViewMatrix();
    Eigen::Matrix4d GetViewMatrixDouble();

    void ProcessKeyboard(Camera_Movement direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);

private:
    void updateCameraVectors();
};

}
