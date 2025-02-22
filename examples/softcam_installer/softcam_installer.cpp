#include <windows.h>
#include <string>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <comdef.h>
#include <olectl.h>
#include <initguid.h>

// {AEF3B972-5FA5-4647-9571-358EB472BC9E}
DEFINE_GUID(CLSID_DShowSoftcam,
0xaef3b972, 0x5fa5, 0x4647, 0x95, 0x71, 0x35, 0x8e, 0xb4, 0x72, 0xbc, 0x9e);

void Message(const std::string& message)
{
    std::cout << message << std::endl;
}


std::string ToHex(long x)
{
    char buff[128];
    std::snprintf(buff, sizeof(buff), "%08lx\n", x);
    return buff;
}


bool IsDllRegistered(const std::string& path)
{
    // Load the DLL first to get its class factory
    HMODULE hmod = LoadLibraryA(path.c_str());
    if (!hmod)
    {
        return false;
    }

    // Get DllGetClassObject function
    using DllGetClassObjectFunc = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, LPVOID*);
    auto DllGetClassObject = (DllGetClassObjectFunc)GetProcAddress(hmod, "DllGetClassObject");
    
    if (!DllGetClassObject)
    {
        FreeLibrary(hmod);
        return false;
    }

    // Try to get the class factory
    IClassFactory* factory = nullptr;
    HRESULT hr = DllGetClassObject(CLSID_DShowSoftcam, IID_IClassFactory, (void**)&factory);
    
    if (factory)
    {
        factory->Release();
    }

    FreeLibrary(hmod);
    return SUCCEEDED(hr);
}


HMODULE LoadDLL(const std::string& path)
{
    HMODULE hmod = LoadLibraryA(path.c_str());
    if (!hmod)
    {
        Message("Error: can't load DLL");
        std::exit(1);
    }
    return hmod;
}


template <typename Func>
Func* GetProc(HMODULE hmod, const std::string& name)
{
    Func* func = (Func*)GetProcAddress(hmod, name.c_str());
    if (!func)
    {
        Message("Error: can't find function " + name + " in DLL");
        std::exit(1);
    }
    return func;
}


int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        Message(
            "Usage:\n"
            "   softcam_installer.exe register <softcam.dll path>\n"
            "   softcam_installer.exe unregister <softcam.dll path>");
        return 0;
    }

    std::string cmd = argv[1], path = argv[2];

    if (cmd == "register")
    {
        // Check if already registered
        if (IsDllRegistered(path))
        {
            Message("softcam.dll is already registered in the system");
            return 0;
        }

        // Not registered, need admin rights to register
        auto hmod = LoadDLL(path);
        auto RegisterServer = GetProc<HRESULT STDAPICALLTYPE()>(hmod, "DllRegisterServer");

        auto hr = RegisterServer();

        if (FAILED(hr))
        {
            Message("Error: registration failed (" + ToHex(hr) + ")");
            return 1;
        }

        Message("softcam.dll has been successfully registered to the system");
        return 0;
    }
    else if (cmd == "unregister")
    {
        // For unregister, we always need admin rights since we don't know if it's registered
        auto hmod = LoadDLL(path);
        auto UnregisterServer = GetProc<HRESULT STDAPICALLTYPE()>(hmod, "DllUnregisterServer");

        auto hr = UnregisterServer();

        if (FAILED(hr))
        {
            Message("Error: unregistration failed (" + ToHex(hr) + ")");
            return 1;
        }

        Message("softcam.dll has been successfully unregistered from the system");
        return 0;
    }
    else
    {
        Message("Error: invalid option");
        return 1;
    }
}
