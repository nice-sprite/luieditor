#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../defines.h"

// filesystem api 
namespace fsapi 
{
    struct FileReadResult
    {
        u64 file_size_bytes;
        void* buffer; 
    };

    bool file_exists(const char* filepath); 
    FileReadResult read_entire_file(const char* path);
    void free_file(FileReadResult file_result); // deallocate the memory we used for this file

//    DirectoryListing list_directory(const char* path);

};

