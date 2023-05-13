#ifndef TEXTURE_H
#define TEXTURE_H
#include "renderer_types.h"
#include <stb/stb_image.h>
#include <d3d11.h>
#include <unordered_map>
#include <string_view>

namespace std::filesystem{ struct directory_entry;};

namespace assets
{
    enum AssetType 
    {
        ImageTexture
    };

    struct Asset 
    {
        std::string id;
        AssetType type;
        union {
            Texture2D texture;
        };
    };

    constexpr std::string_view extensions_to_accept[]{
        ".png",
        ".tiff",
        ".jpg",
        ".jpeg",
    };

    struct AssetDB
    {
        ID3D11Device* device; // for creating images 
        // map of filename -> texture/image
        std::unordered_map<char*, Texture2D> image_cache;
        // directory -> list of its contents
        std::unordered_map<std::string, std::vector<Asset>> fs_cache;
        b8 show_text;
    };

    AssetDB db_init(struct ID3D11Device* device);

    Texture2D load_texture(const char *filename, ID3D11Device *device, bool* success);

    void editor_image_picker(AssetDB* db);

    b8 extension_filter(struct std::filesystem::directory_entry const& dirent);

}

namespace file_dialog
{
    void open();

}

#endif
