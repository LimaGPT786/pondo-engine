#pragma once

#include "../Core.h"
#include <glm/glm.hpp>

namespace Pondo {

#pragma warning(push)
#pragma warning(disable: 4251)
	class PONDO_API Camera {
	public:
		Camera(float fov, float aspectRatio, float nearClip, float farClip);

		void SetPosition(const glm::vec3& position) { m_Position = position; RecalculateViewMatrix(); }
		void SetYaw(float yaw) { m_Yaw = yaw;   RecalculateViewMatrix(); }
		void SetPitch(float pitch) {
			// Clamp pitch so the camera never flips
			if (pitch > 89.0f) pitch = 89.0f;
			if (pitch < -89.0f) pitch = -89.0f;
			m_Pitch = pitch;
			RecalculateViewMatrix();
		}

		const glm::vec3& GetPosition() const { return m_Position; }
		float GetYaw()   const { return m_Yaw; }
		float GetPitch() const { return m_Pitch; }

		const glm::mat4& GetViewMatrix()        const { return m_ViewMatrix; }
		const glm::mat4& GetProjectionMatrix()  const { return m_ProjectionMatrix; }
		const glm::mat4& GetViewProjection()    const { return m_ViewProjectionMatrix; }

		glm::vec3 GetForward() const;
		glm::vec3 GetRight()   const;
		glm::vec3 GetUp()      const;

		// Mouse-look helpers
		void ProcessMouseLook(float deltaX, float deltaY, float sensitivity = 0.15f);

		// Called by EditorLayer when the viewport is resized
		void SetAspectRatio(float aspectRatio);

	private:
		void RecalculateViewMatrix();

		glm::mat4 m_ViewMatrix;
		glm::mat4 m_ProjectionMatrix;
		glm::mat4 m_ViewProjectionMatrix;

		glm::vec3 m_Position = { 0.0f, 1.0f, 5.0f };
		float     m_Yaw = -90.0f;
		float     m_Pitch = 0.0f;
		float     m_Fov;
		float     m_AspectRatio;
		float     m_NearClip;
		float     m_FarClip;
	};
#pragma warning(pop)

}