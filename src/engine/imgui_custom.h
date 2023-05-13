#pragma once
#include <imgui.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
namespace ImGui
{
    // input float with the label on the left
    // also adds a invisible handle on the label, allowing drag left and right to +/- the value
    // sort of like Figma!
    void InputFloat_LeftLabel_Handle(const char* label, float* value);

    // clickable button + image preview to the right
    bool ImagePickerButton(const char* label, ImTextureID preview_img, ImVec2 thumb_size);


	void Mat4x4(glm::mat4& mat);
	void Vec2(glm::vec2& vec);

	void Vec3(glm::vec3& vec);
	void Vec3(glm::vec3& vec);
};
