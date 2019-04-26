// PUID.cpp: implementation of the CPUID class.
//
//////////////////////////////////////////////////////////////////////

#include <Winsock2.h>
#include "windows.h"
#include "wincon.h"
#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include <stddef.h>
#include <nb30.h>
#include <iostream>
#include <string.h>

#include <Assert.h>
#include <winioctl.h>
#include <vector>
#include <list>
#include "Iphlpapi.h"
#include "PUID.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif
#define  MAX_IDE_DRIVES  16

typedef struct CLIENT_INFO
{
    char host_name[128];
    char mac_address[17];
    char gateway[17];
    char ip_address[17];
    char gateway_mac[17];
} client_config_t;

typedef BOOL (WINAPI*DISKID32)(char[],char[]);
#define PRINTING_TO_CONSOLE_ALLOWED

#define  TITLE   "DiskId32"

char HardDriveSerialNumber [1024];
char HardDriveModelNumber [1024];
char RouteMac[18];
int PRINT_DEBUG = false;

static void dump_buffer (const char* title,
                         const unsigned char* buffer,
                         int len);


void WriteConstantString (char *entry,
                          char *string)
{
}


//  Required to ensure correct PhysicalDrive IOCTL structure setup
#pragma pack(1)

#define  IDENTIFY_BUFFER_SIZE  512

//  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
//  see NTDDSCSI.H for definition
#define  IOCTL_SCSI_MINIPORT 0x0004D008

#define SMART_GET_VERSION               CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA            CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

//  GETVERSIONOUTPARAMS contains the data returned from the
//  Get Driver Version function.
typedef struct _GETVERSIONOUTPARAMS
{
    BYTE bVersion;       // Binary driver version.
    BYTE bRevision;      // Binary driver revision.
    BYTE bReserved;      // Not used.
    BYTE bIDEDeviceMap;  // Bit map of IDE devices.
    DWORD fCapabilities; // Bit mask of driver capabilities.
    DWORD dwReserved[4]; // For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;

//  Bits returned in the fCapabilities member of GETVERSIONOUTPARAMS
#define  CAP_IDE_ID_FUNCTION             1  // ATA ID command supported
#define  CAP_IDE_ATAPI_ID                2  // ATAPI ID command supported
#define  CAP_IDE_EXECUTE_SMART_FUNCTION  4  // SMART commannds supported

#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

// The following struct defines the interesting part of the IDENTIFY
// buffer:
typedef struct _IDSECTOR
{
    USHORT  wGenConfig;
    USHORT  wNumCyls;
    USHORT  wReserved;
    USHORT  wNumHeads;
    USHORT  wBytesPerTrack;
    USHORT  wBytesPerSector;
    USHORT  wSectorsPerTrack;
    USHORT  wVendorUnique[3];
    CHAR    sSerialNumber[20];
    USHORT  wBufferType;
    USHORT  wBufferSize;
    USHORT  wECCSize;
    CHAR    sFirmwareRev[8];
    CHAR    sModelNumber[40];
    USHORT  wMoreVendorUnique;
    USHORT  wDoubleWordIO;
    USHORT  wCapabilities;
    USHORT  wReserved1;
    USHORT  wPIOTiming;
    USHORT  wDMATiming;
    USHORT  wBS;
    USHORT  wNumCurrentCyls;
    USHORT  wNumCurrentHeads;
    USHORT  wNumCurrentSectorsPerTrack;
    ULONG   ulCurrentSectorCapacity;
    USHORT  wMultSectorStuff;
    ULONG   ulTotalAddressableSectors;
    USHORT  wSingleWordDMA;
    USHORT  wMultiWordDMA;
    BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;

typedef struct _SRB_IO_CONTROL
{
    ULONG HeaderLength;
    UCHAR Signature[8];
    ULONG Timeout;
    ULONG ControlCode;
    ULONG ReturnCode;
    ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

// Define global buffers.
BYTE IdOutCmd [sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];

char *ConvertToString (DWORD diskdata [256],
                       int firstIndex,
                       int lastIndex,
                       char* buf);

void PrintIdeInfo (int drive,
                   DWORD diskdata [256]);

BOOL DoIDENTIFY (HANDLE,
                 PSENDCMDINPARAMS,
                 PSENDCMDOUTPARAMS,
                 BYTE,
                 BYTE,
                 PDWORD);

// Max number of drives assuming primary/secondary, master/slave topology
#define  MAX_IDE_DRIVES  16

static void getIpAddressByIndex(DWORD ifIndex,
                                char * ip)
{
    DWORD dwSize = 0, i =0;
    if(ERROR_INSUFFICIENT_BUFFER == GetIpAddrTable(NULL, &dwSize,TRUE))
    {
        PMIB_IPADDRTABLE pIPAddrTable = (PMIB_IPADDRTABLE)GlobalAlloc(GPTR, dwSize);
        if(GetIpAddrTable(pIPAddrTable, &dwSize, FALSE) == NO_ERROR)
        {
            for(i = 0; i < pIPAddrTable->dwNumEntries; i++)
            {
                if(ifIndex == pIPAddrTable->table[i].dwIndex)
                {
                    struct in_addr gateway;
                    gateway.s_addr = pIPAddrTable->table[i].dwAddr;
                    if(NULL != ip)
                    {
                        strcpy(ip, inet_ntoa(gateway));
                    }
                }
            }
        }

        GlobalFree(pIPAddrTable);
    }
}

// 获取默认网关IP地址, 以及最近的网络接口索引.
// 返回值: 成功返回 真, 失败返回假
BOOL GetDefaultGateWay(char * sGateWay,
                       char * ipAddress,
                       DWORD* ifIndex)
{
    PMIB_IPFORWARDTABLE pIpRouteTable = NULL;
    DWORD dwActualSize = 0, i = 0, mIfIndex = 0;
    if(ERROR_INSUFFICIENT_BUFFER == GetIpForwardTable(pIpRouteTable, &dwActualSize,TRUE))
    {
        pIpRouteTable = (PMIB_IPFORWARDTABLE)GlobalAlloc(GPTR, dwActualSize);
        if(GetIpForwardTable(pIpRouteTable, &dwActualSize, TRUE) == NO_ERROR)
        {
            DWORD MinMetric = 0x7FFFFFFF, tableIndex = 0;
            MIB_IPFORWARDROW * table = NULL;

            // 遍历查找跳跃点数最小的默认网关.
            for (i = 0; i < pIpRouteTable->dwNumEntries; i++)
            {
                DWORD dwCurrIndex  = pIpRouteTable->table[i].dwForwardIfIndex;

                //	printf("aa=%d\r\n",dwCurrIndex);
                if(0x00 == pIpRouteTable->table[i].dwForwardMask &&
                        0x00 == pIpRouteTable->table[i].dwForwardDest &&
                        3 == pIpRouteTable->table[i].dwForwardProto)
                {
                    if(pIpRouteTable->table[i].dwForwardMetric1 < MinMetric)
                    {
                        MinMetric = pIpRouteTable->table[i].dwForwardMetric1;
                        tableIndex = i;
                        *ifIndex = dwCurrIndex;
                    }
                }
            }

            // printf("ee=%d",*ifIndex);
            if(0x7FFFFFFF != MinMetric)
            {
                struct in_addr gateway;
                table =  &pIpRouteTable->table[tableIndex];
                gateway.s_addr = table->dwForwardNextHop;

                if(NULL != sGateWay)
                {
                    strcpy(sGateWay, inet_ntoa(gateway));
                }

                if(NULL != ipAddress)
                {
                    getIpAddressByIndex(table->dwForwardIfIndex, ipAddress);
                }

                GlobalFree(pIpRouteTable);
                return TRUE;
            }
        }
        GlobalFree(pIpRouteTable);
    }
    return FALSE;
}

static void CheckGateWayMac(client_config_t * _cfg)
{
    unsigned char gateway_mac[6] = {0};
    ULONG   ulen = 6;
    IPAddr destIP = inet_addr(_cfg->gateway);

    if(NO_ERROR == SendARP(destIP, NULL, (PULONG)gateway_mac, &ulen))
    {
        sprintf(_cfg->gateway_mac,
                "%02X%02X%02X%02X%02X%02X",
                gateway_mac[0],
                gateway_mac[1],
                gateway_mac[2],
                gateway_mac[3],
                gateway_mac[4],
                gateway_mac[5]);
    }
}

BOOL getAdapterInfo()
{
    ULONG            cbBuf    = 0;
    PIP_ADAPTER_INFO pAdapter = NULL;
    PIP_ADAPTER_INFO pMemory  = NULL;
    DWORD            dwResult = 0;
    DWORD            ifIndex  = 0;
    client_config_t _cfg;
    char m_gateway[17], m_ipaddress[17];

    // 这里是检测网关.
    if(FALSE == GetDefaultGateWay(m_gateway, m_ipaddress, &ifIndex))
    {
        return FALSE;
    }

    WSADATA wsData;
    ::WSAStartup(MAKEWORD(2, 2), &wsData);
    char szIP[32] = {0};
    char szHostName[32] = {0};

    if(SOCKET_ERROR != gethostname(szHostName, MAX_HOSTNAME_LEN))
    {
        // 成功
        ::WSACleanup();
        dwResult = GetAdaptersInfo(NULL, &cbBuf);

        if(ERROR_BUFFER_OVERFLOW == dwResult)
        {
            pMemory = pAdapter = (PIP_ADAPTER_INFO) malloc(cbBuf > sizeof(IP_ADAPTER_INFO) ?
                                 cbBuf:
                                 sizeof(IP_ADAPTER_INFO));
            if(NO_ERROR == GetAdaptersInfo(pAdapter, &cbBuf))
            {
                while (pAdapter)
                {
                    if(ifIndex == pAdapter->Index)
                    {
                        // 找到了网卡接口MAC
                        sprintf(_cfg.mac_address,
                                "%02X:%02X:%02X:%02X:%02X:%02X",
                                pAdapter->Address[0],
                                pAdapter->Address[1],
                                pAdapter->Address[2],
                                pAdapter->Address[3],
                                pAdapter->Address[4],
                                pAdapter->Address[5]);

                        // 保存网关和IP地址到配置文件.
                        strcpy(_cfg.gateway, m_gateway);
                        strcpy(_cfg.ip_address, m_ipaddress);
                        // 获取网关MAC
                        CheckGateWayMac(&_cfg);
                        strcpy(RouteMac, _cfg.gateway_mac);

                        free(pMemory);
                        return TRUE;
                    }

                    pAdapter = pAdapter->Next;
                }
                free(pMemory);
            }
        }

        return FALSE;
    }
    else
    {
        printf("ERROR:%d\n", GetLastError());
    }
    return FALSE;
}

int ReadPhysicalDriveInNTWithAdminRights(void)
{
    int done = FALSE;
    int drive = 0;

    for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
    {
        HANDLE hPhysicalDriveIOCTL = 0;

        //  Try to get a handle to PhysicalDrive IOCTL, report failure
        //  and exit if can't.
        char driveName [256];

        sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

        //  Windows NT, Windows 2000, must have admin rights
        hPhysicalDriveIOCTL = CreateFileA (driveName,
                                          GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);

        if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
        {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
            if (PRINT_DEBUG)
            {
                printf ("\n%d ReadPhysicalDriveInNTWithAdminRights ERROR"
                        "\nCreateFile(%s) returned INVALID_HANDLE_VALUE\n",
                        __LINE__, driveName);
            }
#endif
        }
        else
        {
            GETVERSIONOUTPARAMS VersionParams;
            DWORD               cbBytesReturned = 0;

            // Get the version, etc of PhysicalDrive IOCTL
            memset ((void*) &VersionParams, 0, sizeof(VersionParams));

            if ( ! DeviceIoControl (hPhysicalDriveIOCTL,
                                    DFP_GET_VERSION,
                                    NULL,
                                    0,
                                    &VersionParams,
                                    sizeof(VersionParams),
                                    &cbBytesReturned,
                                    NULL) )
            {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                if (PRINT_DEBUG)
                {
                    DWORD err = GetLastError ();
                    printf ("\n%d ReadPhysicalDriveInNTWithAdminRights ERROR"
                            "\nDeviceIoControl(%d, DFP_GET_VERSION) returned 0, error is %d\n",
                            __LINE__,
                            (int) hPhysicalDriveIOCTL,
                            (int) err);
                }
#endif
            }

            // If there is a IDE device at number "i" issue commands
            // to the device
            if (VersionParams.bIDEDeviceMap <= 0)
            {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                if (PRINT_DEBUG)
                {
                    printf ("\n%d ReadPhysicalDriveInNTWithAdminRights ERROR"
                            "\nNo device found at position %d (%d)\n",
                            __LINE__,
                            (int) drive,
                            (int) VersionParams.bIDEDeviceMap);
                }
#endif
            }
            else
            {
                BYTE             bIDCmd = 0;   // IDE or ATAPI IDENTIFY cmd
                SENDCMDINPARAMS  scip;

                // Now, get the ID sector for all IDE devices in the system.
                // If the device is ATAPI use the IDE_ATAPI_IDENTIFY command,
                // otherwise use the IDE_ATA_IDENTIFY command
                bIDCmd = (VersionParams.bIDEDeviceMap >> drive & 0x10) ? \
                         IDE_ATAPI_IDENTIFY :
                         IDE_ATA_IDENTIFY;

                memset (&scip, 0, sizeof(scip));
                memset (IdOutCmd, 0, sizeof(IdOutCmd));

                if ( DoIDENTIFY (hPhysicalDriveIOCTL,
                                 &scip,
                                 (PSENDCMDOUTPARAMS)&IdOutCmd,
                                 (BYTE) bIDCmd,
                                 (BYTE) drive,
                                 &cbBytesReturned))
                {
                    DWORD diskdata [256];
                    int ijk = 0;
                    USHORT *pIdSector = (USHORT *)
                                        ((PSENDCMDOUTPARAMS) IdOutCmd) -> bBuffer;

                    for (ijk = 0; ijk < 256; ijk++)
                    {
                        diskdata [ijk] = pIdSector [ijk];
                    }

                    PrintIdeInfo (drive, diskdata);

                    done = TRUE;
                }
            }

            CloseHandle (hPhysicalDriveIOCTL);
        }
    }

    return done;
}

//
// IDENTIFY data (from ATAPI driver source)
//

#pragma pack(1)

typedef struct _IDENTIFY_DATA
{
    USHORT GeneralConfiguration;            // 00 00
    USHORT NumberOfCylinders;               // 02  1
    USHORT Reserved1;                       // 04  2
    USHORT NumberOfHeads;                   // 06  3
    USHORT UnformattedBytesPerTrack;        // 08  4
    USHORT UnformattedBytesPerSector;       // 0A  5
    USHORT SectorsPerTrack;                 // 0C  6
    USHORT VendorUnique1[3];                // 0E  7-9
    USHORT SerialNumber[10];                // 14  10-19
    USHORT BufferType;                      // 28  20
    USHORT BufferSectorSize;                // 2A  21
    USHORT NumberOfEccBytes;                // 2C  22
    USHORT FirmwareRevision[4];             // 2E  23-26
    USHORT ModelNumber[20];                 // 36  27-46
    UCHAR  MaximumBlockTransfer;            // 5E  47
    UCHAR  VendorUnique2;                   // 5F
    USHORT DoubleWordIo;                    // 60  48
    USHORT Capabilities;                    // 62  49
    USHORT Reserved2;                       // 64  50
    UCHAR  VendorUnique3;                   // 66  51
    UCHAR  PioCycleTimingMode;              // 67
    UCHAR  VendorUnique4;                   // 68  52
    UCHAR  DmaCycleTimingMode;              // 69
    USHORT TranslationFieldsValid:1;        // 6A  53
    USHORT Reserved3:15;
    USHORT NumberOfCurrentCylinders;        // 6C  54
    USHORT NumberOfCurrentHeads;            // 6E  55
    USHORT CurrentSectorsPerTrack;          // 70  56
    ULONG  CurrentSectorCapacity;           // 72  57-58
    USHORT CurrentMultiSectorSetting;       //     59
    ULONG  UserAddressableSectors;          //     60-61
    USHORT SingleWordDMASupport : 8;        //     62
    USHORT SingleWordDMAActive : 8;
    USHORT MultiWordDMASupport : 8;         //     63
    USHORT MultiWordDMAActive : 8;
    USHORT AdvancedPIOModes : 8;            //     64
    USHORT Reserved4 : 8;
    USHORT MinimumMWXferCycleTime;          //     65
    USHORT RecommendedMWXferCycleTime;      //     66
    USHORT MinimumPIOCycleTime;             //     67
    USHORT MinimumPIOCycleTimeIORDY;        //     68
    USHORT Reserved5[2];                    //     69-70
    USHORT ReleaseTimeOverlapped;           //     71
    USHORT ReleaseTimeServiceCommand;       //     72
    USHORT MajorRevision;                   //     73
    USHORT MinorRevision;                   //     74
    USHORT Reserved6[50];                   //     75-126
    USHORT SpecialFunctionsEnabled;         //     127
    USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()

int ReadPhysicalDriveInNTUsingSmart (void)
{
    int done = FALSE;
    int drive = 0;

    for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
    {
        HANDLE hPhysicalDriveIOCTL = 0;

        //  Try to get a handle to PhysicalDrive IOCTL, report failure
        //  and exit if can't.
        char driveName [256];

        sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

        //  Windows NT, Windows 2000, Windows Server 2003, Vista
        hPhysicalDriveIOCTL = CreateFileA (driveName,
                                          GENERIC_READ | GENERIC_WRITE,
                                          FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);

        if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
        {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
            if (PRINT_DEBUG)
            {
                printf ("\n%d ReadPhysicalDriveInNTUsingSmart ERROR"
                        "\nCreateFile(%s) returned INVALID_HANDLE_VALUE\n"
                        "Error Code %d\n",
                        __LINE__,
                        driveName,
                        GetLastError ());
            }
#endif
        }
        else
        {
            GETVERSIONINPARAMS GetVersionParams;
            DWORD cbBytesReturned = 0;

            // Get the version, etc of PhysicalDrive IOCTL
            memset ((void*) & GetVersionParams, 0, sizeof(GetVersionParams));

            if ( ! DeviceIoControl (hPhysicalDriveIOCTL,
                                    SMART_GET_VERSION,
                                    NULL,
                                    0,
                                    &GetVersionParams,
                                    sizeof (GETVERSIONINPARAMS),
                                    &cbBytesReturned,
                                    NULL) )
            {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                if (PRINT_DEBUG)
                {
                    DWORD err = GetLastError ();
                    printf ("\n%d ReadPhysicalDriveInNTUsingSmart ERROR"
                            "\nDeviceIoControl(%d, SMART_GET_VERSION) returned 0, error is %d\n",
                            __LINE__,
                            (int) hPhysicalDriveIOCTL,
                            (int) err);
                }
#endif
            }
            else
            {
                // Allocate the command buffer
                ULONG CommandSize = sizeof(SENDCMDINPARAMS) + IDENTIFY_BUFFER_SIZE;
                PSENDCMDINPARAMS Command = (PSENDCMDINPARAMS) malloc (CommandSize);
                // Retrieve the IDENTIFY data
                // Prepare the command
				
				// Returns ID sector for ATA
				#define ID_CMD          0xEC            
                
				Command -> irDriveRegs.bCommandReg = ID_CMD;
                DWORD BytesReturned = 0;
                if ( ! DeviceIoControl (hPhysicalDriveIOCTL,
                                        SMART_RCV_DRIVE_DATA,
                                        Command,
                                        sizeof(SENDCMDINPARAMS),
                                        Command,
                                        CommandSize,
                                        &BytesReturned,
                                        NULL) )
                {
                }
                else
                {
                    // Print the IDENTIFY data
                    DWORD diskdata [256];
                    USHORT *pIdSector = (USHORT *)
                                        (PIDENTIFY_DATA) ((PSENDCMDOUTPARAMS) Command) -> bBuffer;

                    for (int ijk = 0; ijk < 256; ijk++)
                    {
                        diskdata [ijk] = pIdSector [ijk];
                    }

                    PrintIdeInfo (drive, diskdata);
                    done = TRUE;
                }

                // Done
                CloseHandle (hPhysicalDriveIOCTL);
                free (Command);
            }
        }
    }

    return done;
}

//  Required to ensure correct PhysicalDrive IOCTL structure setup
#pragma pack(4)

//
// IOCTL_STORAGE_QUERY_PROPERTY
//
// Input Buffer:
//      a STORAGE_PROPERTY_QUERY structure which describes what type of query
//      is being done, what property is being queried for, and any additional
//      parameters which a particular property query requires.
//
//  Output Buffer:
//      Contains a buffer to place the results of the query into.  Since all
//      property descriptors can be cast into a STORAGE_DESCRIPTOR_HEADER,
//      the IOCTL can be called once with a small buffer then again using
//      a buffer as large as the header reports is necessary.
//

//
// Types of queries
//

//
// Query structure - additional parameters for specific queries can follow
// the header
//


#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Device property descriptor - this is really just a rehash of the inquiry
// data retrieved from a scsi device
//
// This may only be retrieved from a target device.  Sending this to the bus
// will result in an error
//

#pragma pack(4)

//  function to decode the serial numbers of IDE hard drives
//  using the IOCTL_STORAGE_QUERY_PROPERTY command
char * flipAndCodeBytes (const char * str,
                         int pos,
                         int flip,
                         char * buf)
{
    int i;
    int j = 0;
    int k = 0;

    buf [0] = '\0';
    if (pos <= 0)
    {
        return buf;
    }

    if ( ! j)
    {
        char p = 0;

        // First try to gather all characters representing hex digits only.
        j = 1;
        k = 0;
        buf[k] = 0;
        for (i = pos; j && str[i] != '\0'; ++i)
        {
            char c = tolower(str[i]);

            if (isspace(c))
            {
                c = '0';
            }

            ++p;
            buf[k] <<= 4;

            if (c >= '0' && c <= '9')
            {
                buf[k] |= (unsigned char) (c - '0');
            }
            else if (c >= 'a' && c <= 'f')
            {
                buf[k] |= (unsigned char) (c - 'a' + 10);
            }
            else
            {
                j = 0;
                break;
            }

            if (p == 2)
            {
                if (buf[k] != '\0' &&
                        ! isprint(buf[k]))
                {
                    j = 0;
                    break;
                }
                ++k;
                p = 0;
                buf[k] = 0;
            }
        }
    }

    if ( ! j)
    {
        // There are non-digit characters, gather them as is.
        j = 1;
        k = 0;
        for (i = pos; j &&
                str[i] != '\0'; ++i)
        {
            char c = str[i];

            if ( ! isprint(c))
            {
                j = 0;
                break;
            }

            buf[k++] = c;
        }
    }

    if ( ! j)
    {
        // The characters are not there or are not printable.
        k = 0;
    }

    buf[k] = '\0';

    if (flip)
    {
        // Flip adjacent characters
        for (j = 0; j < k; j += 2)
        {
            char t = buf[j];
            buf[j] = buf[j + 1];
            buf[j + 1] = t;
        }
    }

    // Trim any beginning and end space
    i = j = -1;
    for (k = 0; buf[k] != '\0'; ++k)
    {
        if (! isspace(buf[k]))
        {
            if (i < 0)
            {
                i = k;
            }
            j = k;
        }
    }

    if (i >= 0 && 
		j >= 0)
    {
        for (k = i; (k <= j) && (buf[k] != '\0'); ++k)
        {
            buf[k - i] = buf[k];
        }

        buf[k - i] = '\0';
    }

    return buf;
}

#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

int ReadPhysicalDriveInNTWithZeroRights (void)
{
    int done = FALSE;
    int drive = 0;

    for (drive = 0; drive < MAX_IDE_DRIVES; drive++)
    {
        HANDLE hPhysicalDriveIOCTL = 0;

        //  Try to get a handle to PhysicalDrive IOCTL, report failure
        //  and exit if can't.
        char driveName [256];

        sprintf (driveName, "\\\\.\\PhysicalDrive%d", drive);

        //  Windows NT, Windows 2000, Windows XP - admin rights not required
        hPhysicalDriveIOCTL = CreateFileA (driveName,
                                          0,
                                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                                          NULL,
                                          OPEN_EXISTING,
                                          0,
                                          NULL);
        if (hPhysicalDriveIOCTL == INVALID_HANDLE_VALUE)
        {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
            if (PRINT_DEBUG)
            {
                printf ("\n%d ReadPhysicalDriveInNTWithZeroRights ERROR"
                        "\nCreateFile(%s) returned INVALID_HANDLE_VALUE\n",
                        __LINE__,
                        driveName);
            }
#endif
        }
        else
        {
            STORAGE_PROPERTY_QUERY query;
            DWORD cbBytesReturned = 0;
            char buffer [10000];

            memset ((void *) & query, 0, sizeof (query));
            query.PropertyId = StorageDeviceProperty;
            query.QueryType = PropertyStandardQuery;

            memset (buffer, 0, sizeof (buffer));

            if ( DeviceIoControl (hPhysicalDriveIOCTL,
                                  IOCTL_STORAGE_QUERY_PROPERTY,
                                  & query,
                                  sizeof (query),
                                  & buffer,
                                  sizeof (buffer),
                                  & cbBytesReturned,
                                  NULL) )
            {
                STORAGE_DEVICE_DESCRIPTOR * descrip = (STORAGE_DEVICE_DESCRIPTOR *) & buffer;
                char serialNumber [1000];
                char modelNumber [1000];
                char vendorId [1000];
                char productRevision [1000];

#ifdef PRINTING_TO_CONSOLE_ALLOWED
                if (PRINT_DEBUG)
                {
                    printf ("\n%d STORAGE_DEVICE_DESCRIPTOR contents for drive %d\n"
                            "                Version: %ld\n"
                            "                   Size: %ld\n"
                            "             DeviceType: %02x\n"
                            "     DeviceTypeModifier: %02x\n"
                            "         RemovableMedia: %d\n"
                            "        CommandQueueing: %d\n"
                            "         VendorIdOffset: %4ld (0x%02lx)\n"
                            "        ProductIdOffset: %4ld (0x%02lx)\n"
                            "  ProductRevisionOffset: %4ld (0x%02lx)\n"
                            "     SerialNumberOffset: %4ld (0x%02lx)\n"
                            "                BusType: %d\n"
                            "    RawPropertiesLength: %ld\n",
                            __LINE__,
                            drive,
                            (unsigned long) descrip->Version,
                            (unsigned long) descrip->Size,
                            (int) descrip->DeviceType,
                            (int) descrip->DeviceTypeModifier,
                            (int) descrip->RemovableMedia,
                            (int) descrip->CommandQueueing,
                            (unsigned long) descrip->VendorIdOffset,
                            (unsigned long) descrip->VendorIdOffset,
                            (unsigned long) descrip->ProductIdOffset,
                            (unsigned long) descrip->ProductIdOffset,
                            (unsigned long) descrip->ProductRevisionOffset,
                            (unsigned long) descrip->ProductRevisionOffset,
                            (unsigned long) descrip->SerialNumberOffset,
                            (unsigned long) descrip->SerialNumberOffset,
                            (int) descrip->BusType,
                            (unsigned long) descrip->RawPropertiesLength);

                    dump_buffer ("Contents of RawDeviceProperties",
                                 (unsigned char*) descrip->RawDeviceProperties,
                                 descrip->RawPropertiesLength);

                    dump_buffer ("Contents of first 256 bytes in buffer",
                                 (unsigned char*) buffer,
                                 256);
                }
#endif
                flipAndCodeBytes (buffer,
                                  descrip -> VendorIdOffset,
                                  0,
                                  vendorId );
                flipAndCodeBytes (buffer,
                                  descrip -> ProductIdOffset,
                                  0,
                                  modelNumber );
                flipAndCodeBytes (buffer,
                                  descrip -> ProductRevisionOffset,
                                  0,
                                  productRevision );
                flipAndCodeBytes (buffer,
                                  descrip -> SerialNumberOffset,
                                  1,
                                  serialNumber );

                if (0 == HardDriveSerialNumber [0] &&
                        //  serial number must be alphanumeric
                        //  (but there can be leading spaces on IBM drives)
                        (isalnum (serialNumber [0]) || isalnum (serialNumber [19])))
                {
                    strcpy (HardDriveSerialNumber, serialNumber);
                    strcpy (HardDriveModelNumber, modelNumber);
                    done = TRUE;
                }
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                printf ("\n**** STORAGE_DEVICE_DESCRIPTOR for drive %d ****\n"
                        "Vendor Id = [%s]\n"
                        "Product Id = [%s]\n"
                        "Product Revision = [%s]\n"
                        "Serial Number = [%s]\n",
                        drive,
                        vendorId,
                        modelNumber,
                        productRevision,
                        serialNumber);
#endif
                // Get the disk drive geometry.
                memset (buffer, 0, sizeof(buffer));
                if ( ! DeviceIoControl (hPhysicalDriveIOCTL,
                                        IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                        NULL,
                                        0,
                                        &buffer,
                                        sizeof(buffer),
                                        &cbBytesReturned,
                                        NULL))
                {
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                    if (PRINT_DEBUG)
                    {
                        printf ("\n%d ReadPhysicalDriveInNTWithZeroRights ERROR"
                                "|nDeviceIoControl(%s, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX) returned 0",
                                driveName,
								"IOCTL_DISK_GET_DRIVE_GEOMETRY_EX");
                    }
#endif
                }
                else
                {
                    DISK_GEOMETRY_EX* geom = (DISK_GEOMETRY_EX*) &buffer;
                    int fixed = (geom->Geometry.MediaType == FixedMedia);
                    __int64 size = geom->DiskSize.QuadPart;

#ifdef PRINTING_TO_CONSOLE_ALLOWED
                    printf ("\n**** DISK_GEOMETRY_EX for drive %d ****\n"
                            "Disk is%s fixed\n"
                            "DiskSize = %I64d\n",
                            drive,
                            fixed ? "" : " NOT",
                            size);
#endif
                }
            }
            else
            {
                DWORD err = GetLastError ();
#ifdef PRINTING_TO_CONSOLE_ALLOWED
                printf ("\nDeviceIOControl IOCTL_STORAGE_QUERY_PROPERTY error = %d\n", err);
#endif
            }

            CloseHandle (hPhysicalDriveIOCTL);
        }
    }

    return done;
}

// DoIDENTIFY
// FUNCTION: Send an IDENTIFY command to the drive
// bDriveNum = 0-3
// bIDCmd = IDE_ATA_IDENTIFY or IDE_ATAPI_IDENTIFY
BOOL DoIDENTIFY (HANDLE hPhysicalDriveIOCTL,
                 PSENDCMDINPARAMS pSCIP,
                 PSENDCMDOUTPARAMS pSCOP,
                 BYTE bIDCmd,
                 BYTE bDriveNum,
                 PDWORD lpcbBytesReturned)
{
    // Set up data structures for IDENTIFY command.
    pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;
    pSCIP -> irDriveRegs.bFeaturesReg = 0;
    pSCIP -> irDriveRegs.bSectorCountReg = 1;
    pSCIP -> irDriveRegs.bCylLowReg = 0;
    pSCIP -> irDriveRegs.bCylHighReg = 0;

    // Compute the drive number.
    pSCIP -> irDriveRegs.bDriveHeadReg = 0xA0 | ((bDriveNum & 1) << 4);

    // The command can either be IDE identify or ATAPI identify.
    pSCIP -> irDriveRegs.bCommandReg = bIDCmd;
    pSCIP -> bDriveNumber = bDriveNum;
    pSCIP -> cBufferSize = IDENTIFY_BUFFER_SIZE;

    return ( DeviceIoControl (hPhysicalDriveIOCTL,
                              DFP_RECEIVE_DRIVE_DATA,
                              (LPVOID) pSCIP,
                              sizeof(SENDCMDINPARAMS) - 1,
                              (LPVOID) pSCOP,
                              sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1,
                              lpcbBytesReturned,
                              NULL) );
}

//  ---------------------------------------------------

// (* Output Bbuffer for the VxD (rt_IdeDinfo record) *)
typedef struct _rt_IdeDInfo_
{
    BYTE IDEExists[4];
    BYTE DiskExists[8];
    WORD DisksRawInfo[8*256];
} rt_IdeDInfo, *pt_IdeDInfo;


// (* IdeDinfo "data fields" *)
typedef struct _rt_DiskInfo_
{
    BOOL DiskExists;
    BOOL ATAdevice;
    BOOL RemovableDevice;
    WORD TotLogCyl;
    WORD TotLogHeads;
    WORD TotLogSPT;
    char SerialNumber[20];
    char FirmwareRevision[8];
    char ModelNumber[40];
    WORD CurLogCyl;
    WORD CurLogHeads;
    WORD CurLogSPT;
} rt_DiskInfo;


#define  m_cVxDFunctionIdesDInfo  1

//  ---------------------------------------------------

int ReadDrivePortsInWin9X (void)
{
    int done = FALSE;
    unsigned long int i = 0;

    HANDLE VxDHandle = 0;
    pt_IdeDInfo pOutBufVxD = 0;
    DWORD lpBytesReturned = 0;

    //  set the thread priority high so that we get exclusive access to the disk
    BOOL status = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

#ifdef PRINTING_TO_CONSOLE_ALLOWED

    if (0 == status)
    {
        printf ("\nERROR: Could not SetPriorityClass, LastError: %d\n", GetLastError ());
    }
#endif

    // 1. Make an output buffer for the VxD
    rt_IdeDInfo info;
    pOutBufVxD = &info;

    // *****************
    // KLUDGE WARNING!!!
    // HAVE to zero out the buffer space for the IDE information!
    // If this is NOT done then garbage could be in the memory
    // locations indicating if a disk exists or not.
    ZeroMemory (&info, sizeof(info));

    // 1. Try to load the VxD
    VxDHandle = CreateFileA ("\\\\.\\IDE21201.VXD",
                            0,
                            0,
                            0,
                            0,
                            FILE_FLAG_DELETE_ON_CLOSE,
                            0);

    if (VxDHandle != INVALID_HANDLE_VALUE)
    {
        // 2. Run VxD function
        DeviceIoControl (VxDHandle,
                         m_cVxDFunctionIdesDInfo,
                         0,
                         0,
                         pOutBufVxD,
                         sizeof(pt_IdeDInfo),
                         &lpBytesReturned,
                         0);

        // 3. Unload VxD
        CloseHandle (VxDHandle);
    }
    else
    {
        MessageBoxA (NULL,
                    "ERROR: Could not open IDE21201.VXD file",
                    TITLE,
                    MB_ICONSTOP);
    }

    // 4. Translate and store data
    for (i = 0; i < 8; i++)
    {
        if((pOutBufVxD->DiskExists[i]) &&
                (pOutBufVxD->IDEExists[i/2]))
        {
            DWORD diskinfo [256];
            for (int j = 0; j < 256; j++)
            {
                diskinfo [j] = pOutBufVxD -> DisksRawInfo [i * 256 + j];
            }

            // process the information for this buffer
            PrintIdeInfo (i, diskinfo);
            done = TRUE;
        }
    }

    //  reset the thread priority back to normal
    SetPriorityClass (GetCurrentProcess (), NORMAL_PRIORITY_CLASS);

    return done;
}

#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE

int ReadIdeDriveAsScsiDriveInNT (void)
{
    int done = FALSE;
    int controller = 0;

    for (controller = 0; controller < 16; controller++)
    {
        HANDLE hScsiDriveIOCTL = 0;
        char   driveName [256];

        //  Try to get a handle to PhysicalDrive IOCTL, report failure
        //  and exit if can't.
        sprintf (driveName, "\\\\.\\Scsi%d:", controller);

        //  Windows NT, Windows 2000, any rights should do
        hScsiDriveIOCTL = CreateFileA (driveName,
                                      GENERIC_READ | GENERIC_WRITE,
                                      FILE_SHARE_READ | FILE_SHARE_WRITE,
                                      NULL,
                                      OPEN_EXISTING,
                                      0,
                                      NULL);

        if (hScsiDriveIOCTL != INVALID_HANDLE_VALUE)
        {
            int drive = 0;

            for (drive = 0; drive < 2; drive++)
            {
                char buffer [sizeof (SRB_IO_CONTROL) + SENDIDLENGTH];
                SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) buffer;
                SENDCMDINPARAMS *pin =
                    (SENDCMDINPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
                DWORD dummy;

                memset (buffer, 0, sizeof (buffer));
                p->HeaderLength = sizeof (SRB_IO_CONTROL);
                p->Timeout = 10000;
                p->Length = SENDIDLENGTH;
                p->ControlCode = IOCTL_SCSI_MINIPORT_IDENTIFY;
                strncpy ((char *) p->Signature, "SCSIDISK", 8);

                pin->irDriveRegs.bCommandReg = IDE_ATA_IDENTIFY;
                pin->bDriveNumber = drive;

                if(DeviceIoControl(hScsiDriveIOCTL,
                                   IOCTL_SCSI_MINIPORT,
                                   buffer,
                                   sizeof (SRB_IO_CONTROL) +
                                   sizeof (SENDCMDINPARAMS) - 1,
                                   buffer,
                                   sizeof (SRB_IO_CONTROL) + SENDIDLENGTH,
                                   &dummy,
                                   NULL))
                {
                    SENDCMDOUTPARAMS *pOut =
                        (SENDCMDOUTPARAMS *) (buffer + sizeof (SRB_IO_CONTROL));
                    IDSECTOR *pId = (IDSECTOR *) (pOut -> bBuffer);
                    if (pId -> sModelNumber [0])
                    {
                        DWORD diskdata [256];
                        int ijk = 0;
                        USHORT *pIdSector = (USHORT *) pId;

                        for (ijk = 0; ijk < 256; ijk++)
                        {
                            diskdata [ijk] = pIdSector [ijk];
                        }

                        PrintIdeInfo (controller * 2 + drive, diskdata);

                        done = TRUE;
                    }
                }
            }
            CloseHandle (hScsiDriveIOCTL);
        }
    }

    return done;
}

void PrintIdeInfo (int drive,
                   DWORD diskdata [256])
{
    char serialNumber [1024];
    char modelNumber [1024];
    char revisionNumber [1024];
    char bufferSize [32];

    __int64 sectors = 0;
    __int64 bytes = 0;

    //  copy the hard drive serial number to the buffer
    ConvertToString (diskdata, 10, 19, serialNumber);
    ConvertToString (diskdata, 27, 46, modelNumber);
    ConvertToString (diskdata, 23, 26, revisionNumber);
    sprintf (bufferSize, "%u", diskdata [21] * 512);

    if (0 == HardDriveSerialNumber [0] &&
            //  serial number must be alphanumeric
            //  (but there can be leading spaces on IBM drives)
            (isalnum (serialNumber [0]) || isalnum (serialNumber [19])))
    {
        strcpy (HardDriveSerialNumber, serialNumber);
        strcpy (HardDriveModelNumber, modelNumber);
    }

#ifdef PRINTING_TO_CONSOLE_ALLOWED

    printf ("\nDrive %d - ", drive);

    switch (drive / 2)
    {
    case 0:
        printf ("Primary Controller - ");
        break;
    case 1:
        printf ("Secondary Controller - ");
        break;
    case 2:
        printf ("Tertiary Controller - ");
        break;
    case 3:
        printf ("Quaternary Controller - ");
        break;
    }

    switch (drive % 2)
    {
    case 0:
        printf (" - Master drive\n\n");
        break;
    case 1:
        printf (" - Slave drive\n\n");
        break;
    }

    printf ("Drive Model Number________________: [%s]\n",
            modelNumber);
    printf ("Drive Serial Number_______________: [%s]\n",
            serialNumber);
    printf ("Drive Controller Revision Number__: [%s]\n",
            revisionNumber);

    printf ("Controller Buffer Size on Drive___: %s bytes\n",
            bufferSize);

    printf ("Drive Type________________________: ");

    if (diskdata [0] & 0x0080)
    {
        printf ("Removable\n");
    }
    else if (diskdata [0] & 0x0040)
    {
        printf ("Fixed\n");
    }
    else
    {
        printf ("Unknown\n");
    }

    //  calculate size based on 28 bit or 48 bit addressing
    //  48 bit addressing is reflected by bit 10 of word 83
    if (diskdata [83] & 0x400)
    {
        sectors = diskdata [103] * 65536I64 * 65536I64 * 65536I64 +
                  diskdata [102] * 65536I64 * 65536I64 +
                  diskdata [101] * 65536I64 +
                  diskdata [100];
    }
    else
    {
        sectors = diskdata [61] * 65536 + diskdata [60];
    }

    //  there are 512 bytes in a sector
    bytes = sectors * 512;
    printf ("Drive Size________________________: %I64d bytes\n",
            bytes);

#endif  // PRINTING_TO_CONSOLE_ALLOWED

    char string1 [1000];
    sprintf (string1, "Drive%dModelNumber", drive);
    WriteConstantString (string1, modelNumber);

    sprintf (string1, "Drive%dSerialNumber", drive);
    WriteConstantString (string1, serialNumber);

    sprintf (string1, "Drive%dControllerRevisionNumber", drive);
    WriteConstantString (string1, revisionNumber);

    sprintf (string1, "Drive%dControllerBufferSize", drive);
    WriteConstantString (string1, bufferSize);

    sprintf (string1, "Drive%dType", drive);
    if (diskdata [0] & 0x0080)
    {
        WriteConstantString (string1, "Removable");
    }
    else if (diskdata [0] & 0x0040)
    {
        WriteConstantString (string1, "Fixed");
    }
    else
    {
        WriteConstantString (string1, "Unknown");
    }
}

char *ConvertToString (DWORD diskdata [256],
                       int firstIndex,
                       int lastIndex,
                       char* buf)
{
    int index = 0;
    int position = 0;

    //  each integer has two characters stored in it backwards
    for (index = firstIndex; index <= lastIndex; index++)
    {
        //  get high byte for 1st character
        buf [position++] = (char) (diskdata [index] / 256);

        //  get low byte for 2nd character
        buf [position++] = (char) (diskdata [index] % 256);
    }

    //  end the string
    buf[position] = '\0';

    //  cut off the trailing blanks
    for (index = position - 1; index > 0 &&
            isspace(buf [index]); index--)
    {
        buf [index] = '\0';
    }

    return buf;
}

long getHardDriveComputerID ()
{
    int done = FALSE;
    __int64 id = 0;
    OSVERSIONINFO version;

    strcpy (HardDriveSerialNumber, "");

    memset (&version, 0, sizeof (version));
    version.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&version);
    if (version.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        //  this works under WinNT4 or Win2K if you have admin rights
#ifdef PRINTING_TO_CONSOLE_ALLOWED
        printf ("\nTrying to read the drive IDs using physical access with admin rights\n");
#endif
        done = ReadPhysicalDriveInNTWithAdminRights ();

        //  this should work in WinNT or Win2K if previous did not work
        //  this is kind of a backdoor via the SCSI mini port driver into
        //     the IDE drives
#ifdef PRINTING_TO_CONSOLE_ALLOWED
        printf ("\nTrying to read the drive IDs using the SCSI back door\n");
#endif
        // if ( ! done)
        done = ReadIdeDriveAsScsiDriveInNT ();

        //  this works under WinNT4 or Win2K or WinXP if you have any rights
#ifdef PRINTING_TO_CONSOLE_ALLOWED
        printf ("\nTrying to read the drive IDs using physical access with zero rights\n");
#endif
        // if ( ! done)
        done = ReadPhysicalDriveInNTWithZeroRights ();

        //  this works under WinNT4 or Win2K or WinXP or Windows Server 2003 or Vista if you have any rights
#ifdef PRINTING_TO_CONSOLE_ALLOWED
        printf ("\nTrying to read the drive IDs using Smart\n");
#endif
        if ( ! done)
        {
            done = ReadPhysicalDriveInNTUsingSmart ();
        }
    }
    else
    {
        //  this works under Win9X and calls a VXD
        int attempt = 0;

        //  try this up to 10 times to get a hard drive serial number
        for (attempt = 0;
                attempt < 10 && ! done && 0 == HardDriveSerialNumber [0];
                attempt++)
        {
            done = ReadDrivePortsInWin9X ();
        }
    }

    if (HardDriveSerialNumber [0] > 0)
    {
        char *p = HardDriveSerialNumber;

        WriteConstantString ("HardDriveSerialNumber", HardDriveSerialNumber);

        //  ignore first 5 characters from western digital hard drives if
        //  the first four characters are WD-W
        if ( ! strncmp (HardDriveSerialNumber, "WD-W", 4))
        {
            p += 5;
        }

        for ( ; p && *p; p++)
        {
            if ('-' == *p)
            {
                continue;
            }
            id *= 10;
            switch (*p)
            {
            case '0':
                id += 0;
                break;
            case '1':
                id += 1;
                break;
            case '2':
                id += 2;
                break;
            case '3':
                id += 3;
                break;
            case '4':
                id += 4;
                break;
            case '5':
                id += 5;
                break;
            case '6':
                id += 6;
                break;
            case '7':
                id += 7;
                break;
            case '8':
                id += 8;
                break;
            case '9':
                id += 9;
                break;
            case 'a':
            case 'A':
                id += 10;
                break;
            case 'b':
            case 'B':
                id += 11;
                break;
            case 'c':
            case 'C':
                id += 12;
                break;
            case 'd':
            case 'D':
                id += 13;
                break;
            case 'e':
            case 'E':
                id += 14;
                break;
            case 'f':
            case 'F':
                id += 15;
                break;
            case 'g':
            case 'G':
                id += 16;
                break;
            case 'h':
            case 'H':
                id += 17;
                break;
            case 'i':
            case 'I':
                id += 18;
                break;
            case 'j':
            case 'J':
                id += 19;
                break;
            case 'k':
            case 'K':
                id += 20;
                break;
            case 'l':
            case 'L':
                id += 21;
                break;
            case 'm':
            case 'M':
                id += 22;
                break;
            case 'n':
            case 'N':
                id += 23;
                break;
            case 'o':
            case 'O':
                id += 24;
                break;
            case 'p':
            case 'P':
                id += 25;
                break;
            case 'q':
            case 'Q':
                id += 26;
                break;
            case 'r':
            case 'R':
                id += 27;
                break;
            case 's':
            case 'S':
                id += 28;
                break;
            case 't':
            case 'T':
                id += 29;
                break;
            case 'u':
            case 'U':
                id += 30;
                break;
            case 'v':
            case 'V':
                id += 31;
                break;
            case 'w':
            case 'W':
                id += 32;
                break;
            case 'x':
            case 'X':
                id += 33;
                break;
            case 'y':
            case 'Y':
                id += 34;
                break;
            case 'z':
            case 'Z':
                id += 35;
                break;
            }
        }
    }

    id %= 100000000;
    if (strstr (HardDriveModelNumber, "IBM-"))
    {
        id += 300000000;
    }
    else if (strstr (HardDriveModelNumber, "MAXTOR") ||
             strstr (HardDriveModelNumber, "Maxtor"))
    {
        id += 400000000;
    }
    else if (strstr (HardDriveModelNumber, "WDC "))
    {
        id += 500000000;
    }
    else
    {
        id += 600000000;
    }

#ifdef PRINTING_TO_CONSOLE_ALLOWED

    printf ("\nHard Drive Serial Number__________: %s\n",
            HardDriveSerialNumber);
    printf ("\nHard Drive Model Number___________: %s\n",
            HardDriveModelNumber);
    printf ("\nComputer ID_______________________: %I64d\n", id);

#endif

    return (long) id;
}

// GetMACAdapters.cpp : Defines the entry point for the console application.
//
// Author:	Khalid Shaikh [Shake@ShakeNet.com]
// Date:	April 5th, 2002
//
// This program fetches the MAC address of the localhost by fetching the
// information through GetAdapatersInfo.  It does not rely on the NETBIOS
// protocol and the ethernet adapter need not be connect to a network.
//
// Supported in Windows NT/2000/XP
// Supported in Windows 95/98/Me
//
// Supports multiple NIC cards on a PC.

#include "Iphlpapi.h"
#include <Assert.h>

#pragma comment(lib, "..\\lib\\iphlpapi.lib")
#pragma comment(lib, "Wsock32.lib")
#pragma comment(lib, "netapi32.lib")

// Prints the MAC address stored in a 6 byte array to stdout
static void PrintMacAddress(unsigned char MACData[])
{
#ifdef PRINTING_TO_CONSOLE_ALLOWED
    printf("\nMAC Address: %02X-%02X-%02X-%02X-%02X-%02X\n",
           MACData[0],
           MACData[1],
           MACData[2],
           MACData[3],
           MACData[4],
           MACData[5]);
#endif

    char string [256];
    sprintf (string,
             "%02X-%02X-%02X-%02X-%02X-%02X",
             MACData[0],
             MACData[1],
             MACData[2],
             MACData[3],
             MACData[4],
             MACData[5]);
    WriteConstantString ("MACaddress", string);
}

// Fetches the MAC address and prints it
DWORD GetMacAddress(void)
{
    DWORD MACaddress = 0;
    IP_ADAPTER_INFO AdapterInfo[16];       // Allocate information
    // for up to 16 NICs
    DWORD dwBufLen = sizeof(AdapterInfo);  // Save memory size of buffer

    DWORD dwStatus = GetAdaptersInfo(      // Call GetAdapterInfo
                         AdapterInfo,      // [out] buffer to receive data
                         &dwBufLen);       // [in] size of receive data buffer
    assert(dwStatus == ERROR_SUCCESS);     // Verify return value is
    // valid, no buffer overflow

    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; // Contains pointer to
    // current adapter info
    do
    {
        if (MACaddress == 0)
        {
            MACaddress = pAdapterInfo->Address [5] + pAdapterInfo->Address [4] * 256 +
                         pAdapterInfo->Address [3] * 256 * 256 +
                         pAdapterInfo->Address [2] * 256 * 256 * 256;
        }
        PrintMacAddress(pAdapterInfo->Address);	// Print MAC address
        pAdapterInfo = pAdapterInfo->Next;		// Progress through linked list
    }
    while(pAdapterInfo);						// Terminate if last adapter

    return MACaddress;
}

static void dump_buffer (const char* title,
                         const unsigned char* buffer,
                         int len)
{
    int i = 0;
    int j;

    printf ("\n-- %s --\n", title);
    if (len > 0)
    {
        printf ("%8.8s ", " ");
        for (j = 0; j < 16; ++j)
        {
            printf (" %2X", j);
        }
        printf ("  ");
        for (j = 0; j < 16; ++j)
        {
            printf ("%1X", j);
        }
        printf ("\n");
    }

    while (i < len)
    {
        printf("%08x ", i);
        for (j = 0; j < 16; ++j)
        {
            if ((i + j) < len)
            {
                printf (" %02x", (int) buffer[i +j]);
            }
            else
            {
                printf ("   ");
            }
        }
        printf ("  ");
        for (j = 0; j < 16; ++j)
        {
            if ((i + j) < len)
            {
                printf ("%c", isprint (buffer[i + j]) ? buffer [i + j] : '.');
            }
            else
            {
                printf (" ");
            }
        }
        printf ("\n");
        i += 16;
    }
    printf ("-- DONE --\n");
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CPUID::CPUID()
{
	memset(HardDriveSerialNumber, 0x00, sizeof(HardDriveSerialNumber));
	memset(HardDriveModelNumber, 0x00, sizeof(HardDriveModelNumber));
	memset(RouteMac, 0x00, sizeof(RouteMac));
}

CPUID::~CPUID()
{
}

void CPUID::Executecpuid(DWORD veax)
{
    // 因为嵌入式的汇编代码不能识别 类成员变量
    // 所以定义四个临时变量作为过渡
    DWORD deax;
    DWORD debx;
    DWORD decx;
    DWORD dedx;

    __asm
    {
        mov eax, veax ;将输入参数移入eax
        cpuid         ;执行cpuid
        mov deax, eax ;以下四行代码把寄存器中的变量存入临时变量
        mov debx, ebx
        mov decx, ecx
        mov dedx, edx
    }

	// 把临时变量中的内容放入类成员变量
    m_eax = deax; 
    m_ebx = debx;
    m_ecx = decx;
    m_edx = dedx;
}

CString CPUID::GetCpuVID()
{
    // 字符串，用来存储制造商信息
    char cVID[13];
    // 把数组清0
    memset(cVID, 0, 13);
    // 执行cpuid指令，
	// 使用输入参数 eax = 0
    Executecpuid(0);
    // 复制前四个字符到数组
    memcpy(cVID, &m_ebx, 4);
    // 复制中间四个字符到数组
    memcpy(cVID + 4, &m_edx, 4);
    // 复制最后四个字符到数组
    memcpy(cVID + 8, &m_ecx, 4);

    // 以string的形式返回
	
	int num = MultiByteToWideChar(0, 0, cVID, -1, NULL, 0);
	TCHAR *wide_cVID = new TCHAR[num];
	memset(wide_cVID, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, cVID, -1, wide_cVID, num);
	delete []wide_cVID;

    return CString(wide_cVID, num);
}

CString CPUID::GetCpuSerialNumber()
{
	SerialNumber serial;
    // 执行cpuid，参数为 eax = 1
    Executecpuid(1);
    // edx是否为1代表CPU是否存在序列号
    bool isSupport = m_edx & (1 << 18);
    // 不支持，返回false
    if (false == isSupport) { 
		char* strCpuId = "unsupported";
		int num = MultiByteToWideChar(0, 0, strCpuId, -1, NULL, 0);
		TCHAR *wide_CPUId = new TCHAR[num];
		memset(wide_CPUId, 0x00, sizeof(TCHAR) * num);
		MultiByteToWideChar(0, 0, strCpuId, -1, wide_CPUId, num);
		CString CPUID(wide_CPUId, num);
		delete[]wide_CPUId;
		return CPUID;
	}

    // eax为最高位的两个WORD
    memcpy(&serial.nibble[4], &m_eax, 4);

    // 执行cpuid，参数为 eax = 3
    Executecpuid(3);
    // ecx 和 edx为低位的4个WORD
    memcpy(&serial.nibble[0], &m_ecx, 8);

	char strCpuId[256];
	memset(strCpuId, 0x00, 256);
	sprintf(strCpuId, 
		"%08X-%08X-%08X-%08X-%08X-%08X", 
		serial.nibble[0], 
		serial.nibble[1],
		serial.nibble[2],
		serial.nibble[3],
		serial.nibble[4],
		serial.nibble[5]);

	int num = MultiByteToWideChar(0, 0, strCpuId, -1, NULL, 0);
	TCHAR *wide_CPUId = new TCHAR[num];
	memset(wide_CPUId, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, strCpuId, -1, wide_CPUId, num);
	CString CPUID(wide_CPUId, num);
	delete[] wide_CPUId;

    return CPUID;
}

PIP_ADAPTER_INFO pinfo;
unsigned long len = 0;

CString CPUID::getMAC()
{
	char mac[64];
	memset(mac, 0x00, 64);

    NCB ncb;

    typedef struct _ASTAT_ {
        ADAPTER_STATUS adapt;
        NAME_BUFFER NameBuff [30];
    } ASTAT, * PASTAT;
    ASTAT Adapter;

    typedef struct _LANA_ENUM {
        UCHAR length;
        UCHAR lana[MAX_LANA];
    } LANA_ENUM ;
    LANA_ENUM lana_enum;

    UCHAR uRetCode;
    memset( &ncb, 0, sizeof(ncb) );
    memset( &lana_enum, 0, sizeof(lana_enum));

    ncb.ncb_command = NCBENUM;
    ncb.ncb_buffer = (unsigned char *) &lana_enum;
    ncb.ncb_length = sizeof(LANA_ENUM);
    uRetCode = Netbios( &ncb );
    if( uRetCode != NRC_GOODRET ) {
        return uRetCode ;
    }

    for( int lana = 0; lana < lana_enum.length; lana++ ) {
        ncb.ncb_command = NCBRESET;
        ncb.ncb_lana_num = lana_enum.lana[lana];
        uRetCode = Netbios( &ncb );
        if( uRetCode == NRC_GOODRET ) { break ; }
    }

    if( uRetCode != NRC_GOODRET ) {
        return uRetCode;
    }

    memset( &ncb, 0, sizeof(ncb) );
    ncb.ncb_command = NCBASTAT;
    ncb.ncb_lana_num = lana_enum.lana[0];
    strcpy( (char* )ncb.ncb_callname, "*" );
    ncb.ncb_buffer = (unsigned char *) &Adapter;
    ncb.ncb_length = sizeof(Adapter);
    uRetCode = Netbios( &ncb );
    if( uRetCode != NRC_GOODRET ) {
        return uRetCode ;
    }
    sprintf(mac,
            "%02X%02X%02X%02X%02X%02X",
            Adapter.adapt.adapter_address[0],
            Adapter.adapt.adapter_address[1],
            Adapter.adapt.adapter_address[2],
            Adapter.adapt.adapter_address[3],
            Adapter.adapt.adapter_address[4],
            Adapter.adapt.adapter_address[5] );

	int num = MultiByteToWideChar(0, 0, mac, -1, NULL, 0);
	TCHAR *wide_MAC = new TCHAR[num];
	memset(wide_MAC, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, mac, -1, wide_MAC, num);
	CString strMac(wide_MAC, num);
	delete[]wide_MAC;

	return strMac;
}
// 
// 获取网卡MAC地址
// 
CString CPUID::GetMacInfo()
{
	if (pinfo != NULL) {
		// pinfo指针已经指向next了,
		// 不能再简单的释放
		free(pinfo);
		pinfo = NULL;
	}
	unsigned  long nError;
	nError = GetAdaptersInfo(pinfo, &len);

	if (nError == 0) {
		return ParseData();
	}

	if (nError == ERROR_NO_DATA) {
		// AfxMessageBox("No adapter information exists for the local computer");
		return CString();
	}

	if (nError == ERROR_NOT_SUPPORTED) {
		// AfxMessageBox("GetAdaptersInfo is not supported by the operating system running on the local computer");
		return CString();
	}

	if (nError == ERROR_BUFFER_OVERFLOW) {
		pinfo = (PIP_ADAPTER_INFO)malloc(len);
		nError = GetAdaptersInfo(pinfo, &len);
		if (nError == 0) {
			return ParseData();
		}
	}

	return CString();
}

CString CPUID::ParseData()
{
	CString strMac;

	while (pinfo != NULL) {
		int num = MultiByteToWideChar(0, 0, pinfo->Description, -1, NULL, 0);
		TCHAR *wide_pinfo_desc = new TCHAR[num];
		memset(wide_pinfo_desc, 0x00, sizeof(TCHAR) * num);
		MultiByteToWideChar(0, 0, pinfo->Description, -1, wide_pinfo_desc, num);
		CString strPinfoDesc(wide_pinfo_desc, num);
		delete[]wide_pinfo_desc;
		strPinfoDesc.MakeLower();

		if (strPinfoDesc.Find(_TEXT("ppp")) == -1) {
			char mac[64];
			memset(mac, 0x00, 64);
			sprintf(mac,
				"%02X%02X%02X%02X%02X%02X",
				pinfo->Address[0],
				pinfo->Address[1],
				pinfo->Address[2],
				pinfo->Address[3],
				pinfo->Address[4],
				pinfo->Address[5]);

			// IP地址
			char ipAddr[1024];
			memset(ipAddr, 0x00, 1024);
			IP_ADDR_STRING *pIpAddrString = &(pinfo->IpAddressList);
			do {
				sprintf(ipAddr,
					"%s %s",
					ipAddr,
					pIpAddrString->IpAddress.String);
				pIpAddrString = pIpAddrString->Next;
			} while (pIpAddrString);

			switch (pinfo->Type) {
			case MIB_IF_TYPE_OTHER:
				sprintf(mac,
					"%s (%s)",
					mac,
					"OTHER");
				break;
			case MIB_IF_TYPE_ETHERNET:
				sprintf(mac,
					"%s (%s)",
					mac,
					"ETHERNET");
				break;
			case MIB_IF_TYPE_TOKENRING:
				sprintf(mac,
					"%s (%s)",
					mac,
					"TOKENRING");
				break;
			case MIB_IF_TYPE_FDDI:
				sprintf(mac,
					"%s (%s)",
					mac,
					"FDDI");
				break;
			case MIB_IF_TYPE_PPP:
				sprintf(mac,
					"%s (%s)",
					mac,
					"PPP");
				break;
			case MIB_IF_TYPE_LOOPBACK:
				sprintf(mac,
					"%s (%s)",
					mac,
					"LOOPBACK");
				break;
			case MIB_IF_TYPE_SLIP:
				sprintf(mac,
					"%s (%s)",
					mac,
					"SLIP");
				break;
			default:
				sprintf(mac,
					"%s (%u)",
					mac,
					pinfo->Type);
				break;
			}
			// sprintf(mac,
			//	"%s %s",
			//	mac,
			//	ipAddr);

			int num = MultiByteToWideChar(0, 0, mac, -1, NULL, 0);
			TCHAR *wide_MAC = new TCHAR[num];
			memset(wide_MAC, 0x00, sizeof(TCHAR) * num);
			MultiByteToWideChar(0, 0, mac, -1, wide_MAC, num);
			strMac = CString(wide_MAC, num);
			delete[]wide_MAC;

			if (strMac != _TEXT("000000000000")) {
				break;
			}
		}

		pinfo = pinfo->Next;
	}
	return strMac;
}

// 获取CPU序列号
CString CPUID::GetCpuSn()
{	
	unsigned long s1, s2;
	unsigned char vendor_id[] = "                ";
	
	try
	{
		__asm
		{			
			xor eax,eax
			cpuid
			mov dword ptr vendor_id,ebx				
			mov dword ptr vendor_id[+4],edx				
			mov dword ptr vendor_id[+8],ecx				
		}
		
		int num = MultiByteToWideChar(0, 0, (LPCSTR)vendor_id, -1, NULL, 0);
		TCHAR *wide_vendor_id = new TCHAR[num];
		memset(wide_vendor_id, 0x00, sizeof(TCHAR) * num);
		MultiByteToWideChar(0, 0, (LPCSTR)vendor_id, -1, wide_vendor_id, num);
		CString VendorID(wide_vendor_id, num);
		delete []wide_vendor_id;

		VendorID.TrimRight() ;

		__asm
		{
			mov eax,01h
			xor edx,edx
			cpuid
			mov s1,edx
			mov s2,eax
		}
		
		char strCpuId[32];
		memset(strCpuId, 0x00, 32);
		sprintf(strCpuId, "%08X%08X", s1, s2);

		num = MultiByteToWideChar(0, 0, strCpuId, -1, NULL, 0);
		TCHAR *wide_CPUId = new TCHAR[num];
		memset(wide_CPUId, 0x00, sizeof(TCHAR) * num);
		MultiByteToWideChar(0, 0, strCpuId, -1, wide_CPUId, num);
		CString CPUID(wide_CPUId, num);
		delete[]wide_CPUId;

		return CPUID;
	}
	catch (...) 
	{
		return CString(L"unknown");
	}	
}

// 获取硬盘序列号
CString CPUID::getHDSerialNum()
{
    long id = getHardDriveComputerID();
	int num = MultiByteToWideChar(0, 0, HardDriveSerialNumber, -1, NULL, 0);
	TCHAR *wide_HardDriveSerialNumber = new TCHAR[num];
	memset(wide_HardDriveSerialNumber, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, HardDriveSerialNumber, -1, wide_HardDriveSerialNumber, num);
    CString strHdSerialNum(wide_HardDriveSerialNumber, num);
	
	delete []wide_HardDriveSerialNumber;

    strHdSerialNum.TrimLeft();
    return strHdSerialNum;
}

// 获取硬盘型号
CString CPUID::getHDModelNum()
{
    long id = getHardDriveComputerID();

	int num = MultiByteToWideChar(0, 0, HardDriveModelNumber, -1, NULL, 0);
	TCHAR *wide_HardDriveModelNumber = new TCHAR[num];
	memset(wide_HardDriveModelNumber, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, HardDriveModelNumber, -1, wide_HardDriveModelNumber, num);
    CString strModel(wide_HardDriveModelNumber, num);
	delete []wide_HardDriveModelNumber;

    return strModel;
}

// 获取路由Mac
CString CPUID::getRouteMac()
{
	getAdapterInfo();

	int num = MultiByteToWideChar(0, 0, (LPCSTR)RouteMac, -1, NULL, 0);
	TCHAR *wide_RouteMac = new TCHAR[num];
	memset(wide_RouteMac, 0x00, sizeof(TCHAR) * num);
	MultiByteToWideChar(0, 0, (LPCSTR)RouteMac, -1, wide_RouteMac, num);
    CString strRouteMac(wide_RouteMac, num);
	delete []wide_RouteMac;

	return strRouteMac;
}