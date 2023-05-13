#include "win32_lib.h"
#include <Windows.h>
#include <d3d11_4.h>
#include <dwmapi.h>
#include <dxgi1_6.h>
#include <fmt/format.h>
#include <wrl/client.h>

using namespace Microsoft::WRL;
namespace win32 
{
    Window create_window(HINSTANCE hinst,
            const char *windowTitle,
            const char *classname,
            int width,
            int height,
            WNDPROC proc,
            const char *window_icon_path, 
            void* userdata) 
    {
        Window window;
        BOOL attribute_state = TRUE;
        DWORD style = WS_OVERLAPPEDWINDOW;

        WNDCLASS wc = {};
        wc.lpfnWndProc = proc;
        wc.hInstance = hinst;
        wc.lpszClassName = classname;
        wc.hCursor = 0;

        if(window_icon_path)
        {
            wc.hIcon = (HICON)LoadImage(hinst,
                    window_icon_path,
                    IMAGE_ICON,
                    0,
                    0,
                    LR_DEFAULTSIZE | LR_LOADFROMFILE);

        }

        RegisterClass(&wc);

        window.hwnd = CreateWindowEx(0,
                classname,
                windowTitle,
                style,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                width,
                height,
                nullptr,
                nullptr,
                hinst,
                userdata);

        DwmSetWindowAttribute(window.hwnd,
                DWMWA_TRANSITIONS_FORCEDISABLED,
                &attribute_state,
                sizeof(attribute_state));

        ShowWindow(window.hwnd, SW_SHOWDEFAULT);
        UpdateWindow(window.hwnd);
        GetWindowRect(window.hwnd, &window.client_rect);
        return window;
    }

    void set_window_icon(HWND hwnd, const char *iconPath) 
    {
        HINSTANCE hinst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        auto icon = (LONG_PTR)LoadImage(hinst,
                iconPath,
                IMAGE_ICON,
                0,
                0,
                LR_DEFAULTSIZE | LR_LOADFROMFILE);

        SetClassLongPtr(hwnd, GCLP_HICON, icon);
        SetClassLongPtr(hwnd, GCLP_HICONSM, icon);
    }

    // todo move to renderer as this is more related to rendering caps
    DxgiAdapterList get_gpu_specs() 
    {
        ComPtr<IDXGIFactory1> dxgi;
        std::vector<ComPtr<IDXGIAdapter1>> availableAdapters;
        std::vector<DXGI_ADAPTER_DESC1> adapterDefs;
        CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void **)dxgi.GetAddressOf());
        IDXGIAdapter1 *currentAdapter;
        for (UINT i = 0;
                dxgi->EnumAdapters1(i, &currentAdapter) != DXGI_ERROR_NOT_FOUND;
                ++i) {
            ComPtr<IDXGIAdapter1> a = currentAdapter;
            availableAdapters.push_back(a);
            DXGI_ADAPTER_DESC1 desc{};
            a->GetDesc1(&desc);
            adapterDefs.push_back(desc);
        }
        return adapterDefs;
    }

    std::string get_cpu_specs() {
        LPFN_GLPI glpi;
        BOOL done = FALSE;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = nullptr;
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = nullptr;
        DWORD returnLength = 0;
        DWORD logicalProcessorCount = 0;
        DWORD numaNodeCount = 0;
        DWORD processorCoreCount = 0;
        DWORD processorL1CacheCount = 0;
        DWORD processorL2CacheCount = 0;
        DWORD processorL3CacheCount = 0;
        DWORD processorPackageCount = 0;
        DWORD byteOffset = 0;
        PCACHE_DESCRIPTOR Cache;

        glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")),
                "GetLogicalProcessorInformation");
        if (nullptr == glpi) {
            return "\nGetLogicalProcessorInformation is not supported.\n";
        }

        while (!done) {
            DWORD rc = glpi(buffer, &returnLength);

            if (FALSE == rc) {
                if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    if (buffer)
                        free(buffer);

                    buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);

                    if (nullptr == buffer) {
                        return "\nError: Allocation failure\n";
                    }
                } else {
                    return "\nError \n";
                }
            } else {
                done = TRUE;
            }
        }

        ptr = buffer;

        while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <=
                returnLength) {
            switch (ptr->Relationship) {
                case RelationNumaNode:
                    // Non-NUMA systems report a single record of this type.
                    numaNodeCount++;
                    break;

                case RelationProcessorCore:
                    processorCoreCount++;

                    // A hyperthreaded core supplies more than one logical processor.
                    logicalProcessorCount += count_set_bits(ptr->ProcessorMask);
                    break;

                case RelationCache:
                    // Cache data is in ptr->Cache, one CACHE_DESCRIPTOR structure for each
                    // cache.
                    Cache = &ptr->Cache;
                    if (Cache->Level == 1) {
                        processorL1CacheCount++;
                    } else if (Cache->Level == 2) {
                        processorL2CacheCount++;
                    } else if (Cache->Level == 3) {
                        processorL3CacheCount++;
                    }
                    break;

                case RelationProcessorPackage:
                    // Logical processors share a physical package.
                    processorPackageCount++;
                    break;

                default:
                    (TEXT("\nError: Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value.\n"));
                    break;
            }
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr++;
        }

        auto sysCPUInfo = fmt::format("NUMA nodes: {}\n"
                "physical processors: {}\n"
                "processor cores: {}\n"
                "logical processors: {}\n"
                "L1/L2/L3 caches: {}{}{}\n"
                "Cache line size: {}",
                numaNodeCount,
                processorPackageCount,
                processorCoreCount,
                logicalProcessorCount,
                processorL1CacheCount,
                processorL2CacheCount,
                processorL3CacheCount,
                buffer->Cache.LineSize);

        free(buffer);

        return sysCPUInfo;
    }

    double bytes_to_gigabytes(SIZE_T bytes) 
    {
        return (double)bytes / (double)(1 << 30);
    }

    DWORD count_set_bits(ULONG_PTR bitMask) 
    {
        DWORD LSHIFT = sizeof(ULONG_PTR) * 8 - 1;
        DWORD bitSetCount = 0;
        ULONG_PTR bitTest = (ULONG_PTR)1 << LSHIFT;
        DWORD i;

        for (i = 0; i <= LSHIFT; ++i) {
            bitSetCount += ((bitMask & bitTest) ? 1 : 0);
            bitTest /= 2;
        }

        return bitSetCount;
    }


    // TODO move to renderer as well
    std::vector<std::string> dxgi_adapter_list_to_strings(DxgiAdapterList const &adapters) 
    {
        std::vector<std::string> defStrings{};

        constexpr auto formatStr = R"(Description: {}
            VendorId: {}
            DeviceId: {}
            SubSysId: {}
            Revision: {}
            DedicatedVideoMem: {:.2f}
            DedicatedSystemMem: {:.2f}
            SharedSystemMem: {:.2f}
            AdapterLuid: {}
            Flags: {})";

    for (auto &def : adapters) 
    {
        size_t numConverted;
        char cstrDescription[128]{};
        wcstombs_s(&numConverted, cstrDescription, 128, def.Description, _TRUNCATE);
        auto strRep = fmt::format(formatStr,
                cstrDescription,
                def.VendorId,
                def.DeviceId,
                def.SubSysId,
                def.Revision,
                bytes_to_gigabytes(def.DedicatedVideoMemory),
                bytes_to_gigabytes(def.DedicatedSystemMemory),
                bytes_to_gigabytes(def.SharedSystemMemory),
                def.AdapterLuid.LowPart,
                def.Flags);

        defStrings.push_back(strRep);
    }
    return defStrings;
    }

}; // namespace win32
