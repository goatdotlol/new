#ifndef HOOKS_H
#define HOOKS_H

#include <windows.h>
#include <string>

// Driver Communication Codes
#define IOCTL_GET_SERIAL_MAP 0x80002004
#define IOCTL_SPOOF_DISK_ID  0x80002008

struct DRIVER_STATUS {
    bool hooked;
    int error_code;
    char buffer[128];
};

// Internal Spoofer Functions
void MapPhysicalSerials();
void CleanRegistryTraces();
void SyncDriverConfig(); // This will be our hidden heartbeat
void InitializeWatchdog();

#endif
