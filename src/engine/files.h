#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../defines.h"
#include <filesystem>

// remember not to use the stdlib except for a couple things that are hard to 
// do in a platform agnostic way, *and then come back and do the hard part*.
// filesystem api 
namespace fsapi 
{
    using Path = std::filesystem::path;

    struct FileReadResult
    {
        u64 file_size_bytes;
        void* buffer; 
    };

    bool file_exists(const char* filepath); 
    bool file_exists(const Path& filepath); 
    FileReadResult read_entire_file(const char* path);
    FileReadResult read_entire_file(const Path& path);
    void free_file(FileReadResult file_result); // deallocate the memory we used for this file

    Path exe_dir();

    Path parent_path(const Path& filepath);
    
//  DirectoryListing list_directory(const char* path);

};

