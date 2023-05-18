#ifndef TEXTURE_H
#define TEXTURE_H
#include "renderer_types.h"
#include <stb/stb_image.h>
#include <d3d11.h>
#include <unordered_map>
#include <string_view>
#include "files.h"
#include <moodycamel/readerwriterqueue.h>
#define STRINGIFY(n) #n

namespace std::filesystem{ struct directory_entry;};

namespace assets
{
    enum AssetType 
    {
        ImageTexture,
        BrowserThumbmail,
        ImageAtlas,
        Shader,
        RawImageData,
    };

    static inline  
    const char* asset_type_string(AssetType t)
    {
        static const char* names[] {
                STRINGIFY(ImageTexture),
                STRINGIFY(BrowserThumbmail),
                STRINGIFY(ImageAtlas),
                STRINGIFY(Shader),
                STRINGIFY(RawImageData), 
        };
        return names[(int)t];

    }

    struct RawImage
    {
        void* data;
        int width; 
        int height;
        int channels;
    };

    struct Asset 
    {
        AssetType type;
        fsapi::Path path;
        std::string name;  
        union {
            Texture2D texture;
            RawImage raw_image;
        };
    };

    constexpr std::string_view supported_img_types[]{
        ".png",
        ".tiff",
        ".jpg",
        ".jpeg",
    };

    enum AssetJobType
    {
        IMAGE_LOAD,
        DIRECTORY_LOAD,
        SHADER_LOAD,
    };

    struct AssetJob 
    {
        AssetJobType type;
        fsapi::Path path;
    };

    typedef u64 AssetID;

    struct AssetDB
    {
        moodycamel::BlockingReaderWriterQueue<AssetJob>* job_queue;
        HANDLE worker_thread;
        bool has_work;

        fsapi::Path engine_shaders;
        fsapi::Path engine_images;
        ID3D11Device* device; // for creating images 
        // map of filename -> texture/image
        //std::unordered_map<char*, Texture2D> image_cache;
        // directory -> list of its contents
        //std::unordered_map<std::string, std::vector<Asset>> fs_cache;

        // 
        std::unordered_map<AssetID, Asset>          assets;
        std::unordered_map<AssetID, fsapi::Path>    asset_paths;
        std::unordered_map<AssetID, void*>          browser_thumbnails;
        std::unordered_map<fsapi::Path, std::vector<AssetID>> browser_info;
        b8 show_text;
        std::mutex mut;
    };

    AssetDB* db_init(struct ID3D11Device* device);


    // maybe this belongs somewhere else
    // TODO rename to create_texture, and remove path param
    // consumes the RawImage and frees its backing memory
    Texture2D create_texture(RawImage&& raw, ID3D11Device *device, bool* success);

    // returns memory containing pixels of image
    RawImage load_image(const char* filename);
    RawImage load_image(const fsapi::Path& filename);

    void editor_image_picker(AssetDB* db);

    b8 extension_filter(struct std::filesystem::directory_entry const& dirent);

    AssetID compute_hash(fsapi::Path const& asset_path);

    Texture2D* resolve_image(AssetDB* db, AssetID image_id);
    struct GfxShaderProgram* resolve_shader(AssetDB* db, AssetID shader_id);
}

namespace file_dialog
{
    void open();

}

#endif
