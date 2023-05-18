#include "files.h"
//#include <libloaderapi.h>
#include "logging.h"
#include "PathCch.h"
#include "Shlwapi.h"
#include "engine_memory.h"


namespace fsapi 
{

    bool file_exists(const char* filepath)
    {
        DWORD attribs = GetFileAttributes(filepath);

        return (attribs != INVALID_FILE_ATTRIBUTES) && 
           !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    bool file_exists(const Path& filepath) 
    {
        bool exists = std::filesystem::exists(filepath);
        return exists;
    }

    FileReadResult read_entire_file(const char* path)
    {
        HANDLE file_handle;
        DWORD read_bytes; 
        OVERLAPPED ol{0};
        LARGE_INTEGER file_size;
        FileReadResult result{};

        file_handle = CreateFileA(path, 
                GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                0);

        GetFileSizeEx(file_handle, &file_size);

        /* allocate the memory */ 
        void* buffer = engine_alloc("fileio", result.file_size_bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        if( (FALSE == ReadFile(file_handle, buffer, file_size.QuadPart, &read_bytes, &ol)) && (GetLastError() == ERROR_IO_PENDING))
        {
            DWORD overlapped_bytes_completed;
            if(GetOverlappedResult(file_handle, &ol, &overlapped_bytes_completed, true) && overlapped_bytes_completed == result.file_size_bytes)
            {
                result.buffer = buffer;
                result.file_size_bytes = file_size.QuadPart;
            } 
            else
            {
                LOG_WARNING("GetOverlappedResult didnt do well, cleaning memory");
                engine_free(result.buffer, "fileio");
                result.buffer = 0;
                result.file_size_bytes = 0;
            }
        } 

        CloseHandle(file_handle);
        return result; // caller checks if result.buffer != null
    }

    FileReadResult read_entire_file(const Path& path)
    {
        HANDLE file_handle;
        DWORD read_bytes; 
        OVERLAPPED ol{0};
        LARGE_INTEGER file_size;
        FileReadResult result{};

        file_handle = CreateFileW(path.native().c_str(), 
                GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                0);

        GetFileSizeEx(file_handle, &file_size);

        //void* buffer = VirtualAlloc(0, file_size.QuadPart, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        void* buffer = engine_alloc("fileio", file_size.QuadPart, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        if( (FALSE == ReadFile(file_handle, buffer, file_size.QuadPart, &read_bytes, &ol)) && (GetLastError() == ERROR_IO_PENDING))
        {
            DWORD overlapped_bytes_completed;
            if(GetOverlappedResult(file_handle, &ol, &overlapped_bytes_completed, true) && overlapped_bytes_completed == file_size.QuadPart)
            {
                result.file_size_bytes = (u64)file_size.QuadPart;
                result.buffer = buffer;
            } 
            else
            {
                LOG_WARNING("GetOverlappedResult failed, cleaning memory");
                engine_free(result.buffer, "fileio");
                result.buffer = 0; 
            }
        } 

        CloseHandle(file_handle);
        return result;

    }

    void free_file(FileReadResult file_result)
    {
        if(file_result.buffer)
        {
            engine_free(file_result.buffer, "fileio");
            //VirtualFree(file_result.buffer, 0, MEM_RELEASE);
            file_result.file_size_bytes = 0;
        }
    }

    /* TODO: this is really the job of the asset loading system */
    /* temp replacement for figuring out particular directories at runtime */

    // returns the directory the exe was ran from
    // callers responsibility to free the memory for the string when done
    Path exe_dir() 
    {
        char* pathbuf = new char[MAX_PATH];
        GetModuleFileNameA(0, pathbuf, MAX_PATH);
        Path path = pathbuf;
        delete []pathbuf;
        return path.parent_path();
    }

    // returns the current working directory
    // callers responsibility to free the memory for the string when done
    char* current_working_dir()
    {
        char* pathbuf = new char[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, pathbuf);
        return pathbuf;
    }

    Path parent_path(const Path& filepath)
    {
        return filepath.parent_path();
    }



};


