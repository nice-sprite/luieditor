#define STB_IMAGE_IMPLEMENTATION

#include "texture.h"
#include "../defines.h"
#include <imgui.h>
#include <ImFileDialog.h>
#include <string>
#include "logging.h"
#include "engine_memory.h"
#include <synchapi.h>
#include <processthreadsapi.h>
#include <ranges>
#include <algorithm>



// resource loading
namespace assets
{
    template<typename Container,typename V>
    inline bool contains(Container& container, V v) 
    {
        return std::ranges::find(container, v) != std::ranges::end(container);
    }
    
    // maybe return a vector...?
    void load_dir_images(AssetDB* db, fsapi::Path const& dir, bool rec)
    {
        using namespace std::ranges;
        for_each(std::filesystem::directory_iterator{dir}, 
        [&](const auto& dir_entry) 
        {
            auto& filepath = dir_entry.path();
            if(filepath.has_extension() && contains(assets::supported_img_types, filepath.extension()))
            {
                bool good;
                Asset img  
                {
                    .type = AssetType::ImageTexture,
                    .path = filepath,
                    .name = filepath.filename().string(),
                    .texture = create_texture(assets::load_image(filepath), db->device, &good),
                };

                if(good)
                {
                    std::lock_guard lock{db->mut};
                    AssetID id = assets::compute_hash(filepath);
                    db->assets[id] = img;
                    db->browser_info[dir].push_back(id);
                }
            }
        });

    }

    void load_resources(void* assetdb)
    {
        AssetDB* db = (AssetDB*)assetdb;
        static bool not_has_work = false;
        while(1)
        {
            AssetJob job;
            db->job_queue->wait_dequeue(job);
            switch(job.type)
            {
                case IMAGE_LOAD: 
                    {

                    }break;

                case DIRECTORY_LOAD: 
                    {
                        if(std::filesystem::is_directory(job.path))
                        {
                            LOG_INFO("DIRECTORY_LOAD: {}", job.path.string());
                            load_dir_images(db, job.path, false);
                        }
                        else
                        {
                            LOG_INFO("not a directory ({}) ", job.path.string());
                        }
                    } break;
                case SHADER_LOAD: {

                                  }break;
                default: 
                                  break;
            }
            //WaitOnAddress(&db->has_work, &not_has_work, sizeof(bool), INFINITE);
        }

    }


    // pass the device because ImFileDialog needs it for creating textures
    AssetDB* db_init(ID3D11Device* device)
    {
        AssetDB* db = new AssetDB {
            .job_queue = new moodycamel::BlockingReaderWriterQueue<AssetJob>(512),
                .engine_shaders = fsapi::exe_dir().parent_path() / "src/hlsl",
                .engine_images = fsapi::exe_dir().parent_path()/"resource",
                .device = device,
                .show_text = false, 
        };

        db->worker_thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)load_resources, db, 0, 0),
            SetThreadDescription(db->worker_thread, L"ResourceLoadThread");

        ifd::FileDialog::Instance().CreateTexture = 
            [device](u8* data, int w, int h, char fmt) -> void*
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
            if(tex) // this can happen when a large directory is nto finished loading all the preview images
            {
                ((ID3D11ShaderResourceView*)tex)->Release();
            }
        };

        return db;
    }

    fsapi::Path builtin_image(AssetDB* db, fsapi::Path const& path)
    {
        return db->engine_images / path;
    }

    // can have this return the AssetID of the seleceted asset so 
    // calling code can know about images 
    void editor_draw_browser(AssetDB* db)
    {
        static bool good;
        static Texture2D test = create_texture(load_image(builtin_image(db, "spr_bw_350_f.png")), db->device, &good);
        std::lock_guard lock{db->mut};
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec2 thumbnail_size(64, 64);

        float window_vis_x2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        for(auto& [parent_dir, ids] : db->browser_info)
        {
            if(ImGui::CollapsingHeader(parent_dir.string().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                int item_count = 0;
                int total = ids.size();
                for(AssetID id : ids)
                {
                    Asset& asset = db->assets[id];
                    if(asset.type == AssetType::ImageTexture)
                    {
                        ImGui::BeginGroup();
                        ImGui::ImageButton(ImTextureID(asset.texture.srv), thumbnail_size);
                        ImGui::PushTextWrapPos(ImGui::GetItemRectMax().x - ImGui::GetWindowPos().x);
                        ImGui::Text("%s", asset.name.c_str());
                        ImGui::PopTextWrapPos();
                        ImGui::EndGroup();
                        //ImGui::Button("Hi", thumbnail_size);
                        float last_btn_x2 = ImGui::GetItemRectMax().x;
                        float next_btn_x2 = last_btn_x2 + style.ItemSpacing.x + thumbnail_size.x;
                        if(item_count +1 < total && next_btn_x2 < window_vis_x2) {
                            ImGui::SameLine();
                        }
                        ++item_count;
                    }
                }
            }
        }
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
                    std::vector<std::filesystem::path> asset_directories = 
                        ifd::FileDialog::Instance().GetResults();

                    for(std::filesystem::path& dir : asset_directories)
                    {
                        db->job_queue->enqueue(AssetJob {
                                .type = DIRECTORY_LOAD, 
                                .path = dir
                                });
                        /*
                           if(!db->fs_cache.contains(dir.string())) // if its a new directory
                           {
                           for(const auto& dirent : std::filesystem::directory_iterator(dir))
                           {
                           if(dirent.is_regular_file() && extension_filter(dirent))
                           {

                           bool success;
                        // load img as an asset
                        Asset img =  {
                        .id = dirent.path().string(),
                        .type = AssetType::ImageTexture,
                        .texture = assets::create_texture(
                        std::move(assets::load_image(dirent.path())), 
                        db->device, 
                        &success),
                        };
                        db->fs_cache[dir.string()].push_back(img);
                        }
                        }
                        }*/
                    }
                }
                ifd::FileDialog::Instance().Close();
            }

            editor_draw_browser(db);
            // render the asset preview
#if 0           
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
#endif
        }
        ImGui::End();
    }


    Texture2D create_texture(RawImage&& raw, ID3D11Device *device, bool* success) 
    {
        Texture2D texture;

        if(raw.data)
        {
            D3D11_TEXTURE2D_DESC desc {
                .Width = UINT(raw.width),
                    .Height = UINT(raw.height),
                    .MipLevels = 1,
                    .ArraySize = 1,
                    .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .SampleDesc = {
                        .Count = 1,
                    },
                    .Usage = D3D11_USAGE_DEFAULT,
                    .BindFlags = D3D11_BIND_SHADER_RESOURCE,
                    .CPUAccessFlags = 0,
            };

            D3D11_SUBRESOURCE_DATA sr {
                .pSysMem = raw.data,
                    .SysMemPitch = desc.Width * 4,
                    .SysMemSlicePitch = 0,
            };
            device->CreateTexture2D(&desc, &sr, &texture.texture);

            D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{
                .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
                    .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D,
                    .Texture2D = {
                        .MostDetailedMip = 0,
                        .MipLevels = desc.MipLevels,
                    },
            };
            device->CreateShaderResourceView(texture.texture, &srvDesc, &texture.srv);
            *success = 1;
            stbi_image_free(raw.data); // free the raw image from stb
            raw = RawImage{}; // clear to 0
            return texture;
        }
        else 
        {
            *success = 0;
            return Texture2D{};
        }
    }

    RawImage load_image(const char* filename) 
    {
        RawImage image{};
        fsapi::FileReadResult img_file = fsapi::read_entire_file(filename);
        if(img_file.buffer)
        {
            int desired_channels = 4; // want RGBA
            image.data = stbi_load_from_memory(
                    (stbi_uc*)img_file.buffer, 
                    img_file.file_size_bytes,
                    &image.width, 
                    &image.height, 
                    &image.channels, 
                    desired_channels);

            Q_ASSERTMSG(image.data != 0, "shit! damitahh!!");
            fsapi::free_file(img_file); // free the raw disk data
            return image;
        }
        else
        {
            fsapi::free_file(img_file); 
            return {};
        }
    }

    RawImage load_image(const fsapi::Path& filename) 
    {
        RawImage image{};
        fsapi::FileReadResult img_file;
        ZoneScopedN("load_image");
        {
            ZoneScopedN("read_entire_file");
            img_file = fsapi::read_entire_file(filename);
        }

        if(img_file.buffer)
        {
            int desired_channels = 4; // want RGBA
            {
                ZoneScopedN("stbi_load_from_memory");
                image.data = stbi_load_from_memory(
                        (stbi_uc*)img_file.buffer, 
                        img_file.file_size_bytes,
                        &image.width, 
                        &image.height, 
                        &image.channels, 
                        desired_channels);
            }
            Q_ASSERTMSG(image.data != 0, "shit! damitahh!!");
            fsapi::free_file(img_file); 
            return image;
        }
        else
        {
            fsapi::free_file(img_file); 
            return {};
        }
    }


    // https://stackoverflow.com/a/3771375
    AssetID compute_hash(fsapi::Path const& asset_path)
    {
        return std::filesystem::hash_value(asset_path);
        /*u64 hash = 10000019;
          for(auto& v : asset_path.string())
          {
          LOG_INFO("hashing {}", v);
          }
          */

    }

    Texture2D* resolve_image(AssetDB* db, AssetID image_id)
    {
        if(db->assets.contains(image_id) && db->assets[image_id].type == ImageTexture)
        {
            return &db->assets[image_id].texture;
        }
        else
        {
            return 0;
        }
    }

    struct GfxShaderProgram* resolve_shader(AssetDB* db, AssetID shader_id)
    {
        if(db->assets.contains(shader_id) && 
                db->assets[shader_id].type == ImageTexture)
        {
            //return &db->assets[shader_id].shader;
            return 0;
        }
        else
        {
            return 0;
        }

    }
}

