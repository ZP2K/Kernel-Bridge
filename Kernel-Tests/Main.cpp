#include "pch.h"

#include "WdkTypes.h"
#include "CtlTypes.h"
#include "FltTypes.h"
#include "User-Bridge.h"
#include "Rtl-Bridge.h"

#include <fltUser.h>
#include "CommPort.h"
#include "Flt-Bridge.h"

#include "Kernel-Tests.h"

#include <vector>
#include <iostream>

void RunTests() {
    BeeperTest tBeeper(L"Beeper");
    IoplTest tIopl(L"IOPL");
    VirtualMemoryTest tVirtualMemory(L"VirtualMemory");
    MdlTest tMdl(L"Mdl");
    PhysicalMemoryTest tPhysicalMemory(L"PhysicalMemory");
    ProcessesTest tProcesses(L"Processes");
    ShellTest tShell(L"Shells");
    StuffTest tStuff(L"Stuff");
}

class FilesReader final {
private:
    PVOID Buffer;
    ULONG Size;
public:
    FilesReader(LPCWSTR FilePath) {
        HANDLE hFile = CreateFile(FilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) throw std::runtime_error("File not found!");
        ULONG Size = GetFileSize(hFile, NULL);
        BOOL Status = FALSE;
        if (Size) {
            Buffer = VirtualAlloc(NULL, Size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (!Buffer) {
                CloseHandle(hFile);
                throw std::runtime_error("Memory not allocated!");
            }
            Status = ReadFile(hFile, Buffer, Size, &Size, NULL);
        }
        CloseHandle(hFile);
        if (!Status) throw std::runtime_error("Reading failure!");
    }
    ~FilesReader() {
        if (Buffer) VirtualFree(Buffer, 0, MEM_RELEASE);
    }
    PVOID Get() const { return Buffer; }
    ULONG GetSize() const { return Size; }
};

void MapDriver(LPCWSTR Path) {
    FilesReader DriverImage(Path);
    KbRtl::KbMapDriver(DriverImage.Get(), L"\\Driver\\EnjoyTheRing0");
}

#define TEST_CALLBACKS

int main() {
    KbLoader::KbUnload();
    if (KbLoader::KbLoadAsFilter(
        L"C:\\Temp\\Kernel-Bridge\\Kernel-Bridge.sys",
        L"260000" // Altitude of minifilter
    )) {
#ifdef TEST_CALLBACKS
        BOOL Status = TRUE;

/*
        CommPortListener<KB_FLT_OB_CALLBACK_INFO, KbObCallbacks> Callbacks;

        Status = Callbacks.Subscribe([](CommPort& Port, MessagePacket<KB_FLT_OB_CALLBACK_INFO>& Message) -> VOID {
            auto Data = static_cast<PKB_FLT_OB_CALLBACK_INFO>(Message.GetData());
            if (Data->Target.ProcessId == GetCurrentProcessId()) {
                Data->CreateResultAccess = 0;
                Data->DuplicateResultAccess = 0;
            }
            ReplyPacket<KB_FLT_OB_CALLBACK_INFO> Reply(Message, ERROR_SUCCESS, *Data);
            Port.Reply(Reply);
        });
*/
        CommPortListener<KB_FLT_PS_IMAGE_INFO, KbPsImage> ProcessCallbacks;

        Status = ProcessCallbacks.Subscribe([](CommPort& Port, MessagePacket<KB_FLT_PS_IMAGE_INFO>& Message) -> VOID {
            auto Data = static_cast<PKB_FLT_PS_IMAGE_INFO>(Message.GetData());
            printf("[PID: %i | Base: 0x%I64X | Size: %ull]: %ws\r\n", (int)Data->ProcessId, Data->BaseAddress, (int)Data->ImageSize, Data->FullImageName);
        });

        if (!Status) std::cout << "Callbacks failure!" << std::endl;
        while (true) Sleep(100);
#endif

        RunTests();
        KbLoader::KbUnload();
    } else {
        std::wcout << L"Unable to load driver!" << std::endl;
    }

    std::wcout << L"Press any key to exit..." << std::endl;
    std::cin.get();
}