#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace Pondo {

	Camera::Camera(float fov, float aspectRatio, float nearClip, float farClip)
		: m_Fov(fov), m_AspectRatio(aspectRatio),
		m_NearClip(nearClip), m_FarClip(farClip)
	{
		m_ProjectionMatrix = glm::perspective(
			glm::radians(fov), aspectRatio, nearClip, farClip);
		RecalculateViewMatrix();
	}

	void Camera::SetAspectRatio(float aspectRatio)
	{
		m_AspectRatio = aspectRatio;
		m_ProjectionMatrix = glm::perspective(
			glm::radians(m_Fov), aspectRatio, m_NearClip, m_FarClip);
		RecalculateViewMatrix();
	}

	glm::vec3 Camera::GetForward() const
	{
		return glm::normalize(glm::vec3{
			cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
			sin(glm::radians(m_Pitch)),
			sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))
			});
	}

	glm::vec3 Camera::GetRight() const
	{
		return glm::normalize(glm::cross(GetForward(), glm::vec3(0, 1, 0)));
	}

	glm::vec3 Camera::GetUp() const
	{
		return glm::normalize(glm::cross(GetRight(), GetForward()));
	}

	void Camera::ProcessMouseLook(float deltaX, float deltaY, float sensitivity)
	{
		m_Yaw += deltaX * sensitivity;
		float newPitch = m_Pitch - deltaY * sensitivity;
		if (newPitch > 89.0f) newPitch = 89.0f;
		if (newPitch < -89.0f) newPitch = -89.0f;
		m_Pitch = newPitch;
		RecalculateViewMatrix();
	}

	void Camera::RecalculateViewMatrix()
	{
		m_ViewMatrix = glm::lookAt(
			m_Position,
			m_Position + GetForward(),
			glm::vec3(0, 1, 0));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}

}