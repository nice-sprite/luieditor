#include "files.h"
//#include <libloaderapi.h>
#include "logging.h"

namespace fsapi 
{
    bool file_exists(const char* filepath)
    {
        DWORD attribs = GetFileAttributes(filepath);

        return (attribs != INVALID_FILE_ATTRIBUTES) && 
           !(attribs & FILE_ATTRIBUTE_DIRECTORY);
    }

    FileReadResult read_entire_file(const char* path)
    {
        HANDLE file_handle;
        DWORD read_bytes; 
        FileReadResult result{};
        OVERLAPPED ol{0};
        LARGE_INTEGER file_size;

        file_handle = CreateFileA(path, 
                GENERIC_READ,
                FILE_SHARE_READ,
                0,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                0);

        GetFileSizeEx(file_handle, &file_size);
        result.file_size_bytes = file_size.QuadPart;

        /* allocate the memory */ 
        result.buffer = VirtualAlloc(0, result.file_size_bytes, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
        result.file_size_bytes = file_size.QuadPart;
        if( (FALSE == ReadFile(file_handle, result.buffer, result.file_size_bytes, &read_bytes, &ol)) && 
                (GetLastError() == ERROR_IO_PENDING))
        {
            DWORD overlapped_bytes_completed;
            if(GetOverlappedResult(file_handle, &ol, &overlapped_bytes_completed, true))
            {
                if(overlapped_bytes_completed == result.file_size_bytes)
                {
                    return(result);
                }
                else
                {
                    return FileReadResult{}; 
                }
            } 
            else
            {
                LOG_WARNING("GetOverlappedResult didnt do well");
                return FileReadResult{};

            }
        } 
        else
        { 
            return FileReadResult{}; 
        }

    }

    void free_file(FileReadResult file_result)
    {
        if(file_result.buffer)
        {
            VirtualFree(file_result.buffer, 0, MEM_RELEASE);
            file_result.file_size_bytes = 0;
        }
    }

    /* TODO: this is really the job of the asset loading system */
    /* temp replacement for figuring out particular directories at runtime */

    // returns the directory the exe was ran from
    // callers responsibility to free the memory for the string when done
    char* exe_dir() 
    {
        char* pathbuf = new char[MAX_PATH];
        GetModuleFileNameA(0, pathbuf, MAX_PATH);
        return pathbuf;
    }

    // returns the current working directory
    // callers responsibility to free the memory for the string when done
    char* current_working_dir()
    {
        char* pathbuf = new char[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, pathbuf);
        return pathbuf;
    }


};


