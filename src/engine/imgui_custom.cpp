#include "imgui_custom.h"
#include <imgui_internal.h>
#include <fmt/core.h>
#include <fmt/format.h>




namespace ImGui {

    void InputFloat_LeftLabel_Handle(const char* label, float* value)
    {

        ImGui::BeginGroup();
        ImGuiID storage_id = ImGui::GetID(label);
        ImGui::Text("%s", label);   

        if (ImGui::IsItemHovered())
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if(ImGui::IsItemClicked())
            ImGui::GetStateStorage()->SetBool(storage_id, true);

        if(ImGui::IsMouseReleased(ImGuiMouseButton_Left))
            ImGui::GetStateStorage()->SetBool(storage_id, false);
            
        if(ImGui::GetStateStorage()->GetBool(storage_id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            ImVec2 drag_delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
            *value += drag_delta.x;
            ImGui::ResetMouseDragDelta(ImGuiMouseButton_Left);
        }

        ImGui::SameLine();
        ImGui::InputFloat(fmt::format("##{}", label).c_str(), value);
        ImGui::EndGroup();
    }

    bool ImagePickerButton(const char* label, ImTextureID preview_img, ImVec2 thumb_size)
    {
        bool ret = 0;
        ret = ImGui::SmallButton(label);
        ImGui::SameLine();
        return ImGui::ImageButton(preview_img, thumb_size, ImVec2(0, 0), ImVec2(1, 1), 1.0f, ImVec4(1.0, 1.0, 1.0, 1.0)) | ret;
    }

    void Mat4x4(glm::mat4& mat)
    {
        ImGui::Text("%.2f %.2f %.2f %.2f", mat[0].x, mat[0].y, mat[0].z, mat[0].w);
        ImGui::Text("%.2f %.2f %.2f %.2f", mat[1].x, mat[1].y, mat[1].z, mat[1].w);
        ImGui::Text("%.2f %.2f %.2f %.2f", mat[2].x, mat[2].y, mat[2].z, mat[2].w);
        ImGui::Text("%.2f %.2f %.2f %.2f", mat[3].x, mat[3].y, mat[3].z, mat[3].w);
    }

    void Vec2(glm::vec2& vec)
    {
        ImGui::Text("(%.2f, %.2f)", vec.x, vec.y);
    }

    void Vec3(glm::vec3& vec)
    {
        ImGui::Text("(%.2f, %.2f %.2f)", vec.x, vec.y, vec.z);
    }

    void Vec4(glm::vec4& vec)
    {
        ImGui::Text("(%.2f, %.2f %.2f %.2f)", vec.x, vec.y, vec.z, vec.w);
    }
}
