#define STB_IMAGE_IMPLEMENTATION

#include "Texture.h"
#include "../defines.h"
#include <imgui.h>
#include <ImFileDialog.h>
#include <string>
#include "logging.h"

namespace assets
{
    // pass the device because ImFileDialog needs it for creating textures
    AssetDB db_init(ID3D11Device* device)
    {
        AssetDB db {
            .device = device,
            .show_text = false,
        };

        ifd::FileDialog::Instance().CreateTexture = [device](u8* data, int w, int h, char fmt)-> void*
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = w;
            desc.Height = h;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            ID3D11Texture2D* texture; // I think can free this?
            D3D11_SUBRESOURCE_DATA sr;
            sr.pSysMem = data;
            sr.SysMemPitch = desc.Width * 4;
            sr.SysMemSlicePitch = 0;
            device->CreateTexture2D(&desc, &sr, &texture);

            ID3D11ShaderResourceView* srv;
            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            device->CreateShaderResourceView(texture, &srvDesc, &srv);

            texture->Release();

            return (void*)srv;
        };

        ifd::FileDialog::Instance().DeleteTexture = [](void* tex) {
            if(tex)
            {
                ((ID3D11ShaderResourceView*)tex)->Release();
            }
        };

        return db;
    }

    void editor_image_picker(AssetDB* db)
    {
        if(ImGui::Begin("Image picker"))
        {
            ImGui::Checkbox("Show Filename", &db->show_text);


            if(ImGui::Button("Attach to asset directory"))
            {
                ifd::FileDialog::Instance().Open( "filedialog", "Select Asset Directory", "", true);
            }


            if (ifd::FileDialog::Instance().IsDone("filedialog"))
            {
                if(ifd::FileDialog::Instance().HasResult())
                {
                    // TODO break this loading out into a thread/multiple threads
                    // so that we dont block main render thread to load shit
                    std::vector<std::filesystem::path> asset_directories = ifd::FileDialog::Instance().GetResults();
                    for(std::filesystem::path& dir : asset_directories)
                    {
                        if(!db->fs_cache.contains(dir.string())) // if its a new directory
                        {
                            for(const auto& dirent : std::filesystem::directory_iterator(dir))
                            {
                                if(dirent.is_regular_file() && extension_filter(dirent))
                                {
                                    // load img as an asset
                                    Asset img;
                                    bool success;
                                    img.id = dirent.path().string();
                                    img.type = AssetType::ImageTexture;
                                    img.texture = load_texture(dirent.path().string().c_str(), db->device, &success);
                                    db->fs_cache[dir.string()].push_back(img);
                                }
                            }
                        }
                    }
                }
                ifd::FileDialog::Instance().Close();
            }

            // render the asset preview
            for(const auto&[toplevel_directory, contents] : db->fs_cache)
            {
                if(ImGui::CollapsingHeader(toplevel_directory.c_str()))
                {
                    // TODO this calculates a slightly janky layout but thats low priority atm
                    ImVec2 available_space = ImGui::GetContentRegionAvail();
                    f32 cell_padding = ImGui::GetStyle().ColumnsMinSpacing;
                    f32 thumbnail_size = 64.f;
                    i32 img_per_row = glm::max((i32)glm::floor((available_space.x + 2.f*cell_padding) / thumbnail_size), 1); // prevent divide by 0 and good behaviour for small width window
                    i32 num_rows = (i32)glm::floor(available_space.y / thumbnail_size);

                    ImGui::Columns(img_per_row, NULL, 0);

                    int count = 0;
                    for(const Asset& asset : contents)
                    {
                        ImGui::BeginGroup();
                        ImVec2 size{(f32)asset.texture.width, (f32)asset.texture.height};
                        ImGui::ImageButton((void*)asset.texture.srv, ImVec2(thumbnail_size, thumbnail_size));
                        if(db->show_text)
                        {
                            ImGui::TextWrapped("%s", asset.id.c_str());
                        }
                        ImGui::EndGroup();
                        ImGui::NextColumn();
                        count++;
                    }
                    ImGui::Columns(1);
                }
            }
        }
        ImGui::End();
    }


    Texture2D load_texture(const char *filename, ID3D11Device *device, bool* success) 
    {
        Texture2D texture;
        // Load from disk into a raw RGBA buffer
        unsigned char *image_data = stbi_load(filename, 
                (i32*)&texture.width, 
                (i32*)&texture.height, 
                NULL, 
                4);

        if(image_data)
        {
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = texture.width;
            desc.Height = texture.height;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            desc.CPUAccessFlags = 0;

            D3D11_SUBRESOURCE_DATA sr;
            sr.pSysMem = image_data;
            sr.SysMemPitch = desc.Width * 4;
            sr.SysMemSlicePitch = 0;
            device->CreateTexture2D(&desc, &sr, &texture.texture);

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = desc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            device->CreateShaderResourceView(texture.texture, &srvDesc, &texture.srv);
            stbi_image_free(image_data);
            *success = 1;
            return texture;

        }
        else 
        {
            *success = 0;
            return Texture2D{};
        }
    }

    b8 extension_filter(std::filesystem::directory_entry const& dirent)
    {
        for(const auto& extension : assets::extensions_to_accept)
        {
            // TODO: this might be doing some stupid conversion
            // to check equality...
            if(dirent.path().extension() == extension) return true;
        }
        return false;
    }
}

