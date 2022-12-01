#include <intrin.h>
#include <iostream>
#include <Windows.h>
#include <sysinfoapi.h>
#include <powerbase.h>
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "Powrprof.lib")

typedef struct _PROCESSOR_POWER_INFORMATION {
  ULONG Number;
  ULONG MaxMhz;
  ULONG CurrentMhz;
  ULONG MhzLimit;
  ULONG MaxIdleState;
  ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION, *PPROCESSOR_POWER_INFORMATION;

char* assoc0 = "0";
const char* E_4(unsigned char assoc) {
    if (assoc < 4) {
        *assoc0 = 0x30 + assoc;
        return assoc0;
    }
    if (assoc == 4) return "4 to 5";
    if (assoc == 5) return "6 to 7";
    if (assoc == 6) return "8 to 15";
    if (assoc == 8) return "16 to 31";
    return "unknown";
}

int main() {
    std::cout << "/* generated with 2a.cpp */" << std::endl << std::endl;

    int regs[5];
    regs[4] = 0;

    __cpuid(regs, 0x80000002);
    std::cout << "// " << (char*)regs;

    __cpuid(regs, 0x80000003);
    std::cout << (char*)regs;

    __cpuid(regs, 0x80000004);
    std::cout << (char*)regs << std::endl;

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);
    PROCESSOR_POWER_INFORMATION* ppi = new PROCESSOR_POWER_INFORMATION[sysinfo.dwNumberOfProcessors];
    CallNtPowerInformation(ProcessorInformation, NULL, 0, ppi, sysinfo.dwNumberOfProcessors * sizeof(*ppi));

    std::cout << "// " << sysinfo.dwNumberOfProcessors << " threads @ " << ppi[0].CurrentMhz << "MHz" << std::endl;

    delete ppi;

    HKEY hKey;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey);

    wchar_t osname[256];
    DWORD size = sizeof(osname);
    RegQueryValueExW(hKey, L"ProductName", NULL, NULL, (LPBYTE)osname, &size);

    std::cout << "// ";
    std::wcout << osname << std::endl << std::endl;

    __cpuid(regs, 0x80000005);
    std::cout << "// L1I: " << ((regs[3] >> 24) & 0xFF) << "kB per core, " << ((regs[3] >> 16) & 0xFF) << "-way associativity, " << ((regs[3] >> 8) & 0xFF) << " lines per tag, " << (regs[3] & 0xFF) << "B line size" << std::endl;
    std::cout << "// L1D: " << ((regs[2] >> 24) & 0xFF) << "kB per core, " << ((regs[2] >> 16) & 0xFF) << "-way associativity, " << ((regs[2] >> 8) & 0xFF) << " lines per tag, " << (regs[2] & 0xFF) << "B line size" << std::endl;
    std::cout << "#define S1 (" << ((regs[2] >> 24) & 0xFF) << "*1024)" << std::endl;
    std::cout << "#define B1 (" << (regs[2] & 0xFF) << ")" << std::endl << std::endl;

    __cpuid(regs, 0x80000006);
    std::cout << "// L2: " << ((regs[2] >> 16) & 0xFFFF) << "kB per core, " << E_4((regs[2] >> 12) & 0xF) << "-way associativity, " << ((regs[2] >> 8) & 0xF) << " lines per tag, " << (regs[2] & 0xFF) << "B line size" << std::endl;
    std::cout << "#define S2 (" << ((regs[2] >> 16) & 0xFFFF) << "*1024)" << std::endl;
    std::cout << "#define B2 (" << (regs[2] & 0xFF) << ")" << std::endl << std::endl;

    std::cout << "// L3: " << ((regs[3] >> 18) & 0xFFFF) / 2.0 << "MB total, " << E_4((regs[3] >> 12) & 0xF) << "-way associativity, " << ((regs[3] >> 8) & 0xF) << " lines per tag, " << (regs[3] & 0xFF) << "B line size" << std::endl;
    std::cout << "#define S3 (" << ((regs[3] >> 18) & 0xFFFF) << "*512*1024)" << std::endl;
    std::cout << "#define B3 (" << (regs[3] & 0xFF) << ")" << std::endl << std::endl;

    return 0;
}
