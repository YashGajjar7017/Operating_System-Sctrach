/**
 * @file efi.h
 * @brief Complete Freestanding UEFI 2.8 Specification Header Definitions for x86_64
 */

#ifndef _BOOTLOADER_EFI_H_
#define _BOOTLOADER_EFI_H_

#include <stdint.h>
#include <stddef.h>

#if defined(_MSC_VER)
    #define EFIAPI __cdecl
#else
    #define EFIAPI __attribute__((ms_abi))
#endif

#define IN
#define OUT
#define OPTIONAL
#define CONST const

/* Basic Types */
typedef uint8_t   BOOLEAN;
typedef int64_t   INTN;
typedef uint64_t  UINTN;
typedef int8_t    INT8;
typedef uint8_t   UINT8;
typedef int16_t   INT16;
typedef uint16_t  UINT16;
typedef int32_t   INT32;
typedef uint32_t  UINT32;
typedef int64_t   INT64;
typedef uint64_t  UINT64;
typedef char      CHAR8;
typedef uint16_t  CHAR16;
typedef void      VOID;

typedef UINTN     EFI_STATUS;
typedef VOID*     EFI_HANDLE;
typedef VOID*     EFI_EVENT;
typedef UINT64    EFI_LBA;
typedef UINTN     EFI_TPL;

typedef UINT64    EFI_PHYSICAL_ADDRESS;
typedef UINT64    EFI_VIRTUAL_ADDRESS;

#define TRUE  1
#define FALSE 0
#define NULL  ((VOID*)0)

/* Status Codes */
#define EFI_SUCCESS               0ULL
#define EFI_LOAD_ERROR            (1ULL | (1ULL << 63))
#define EFI_INVALID_PARAMETER     (2ULL | (1ULL << 63))
#define EFI_UNSUPPORTED           (3ULL | (1ULL << 63))
#define EFI_BAD_BUFFER_SIZE       (4ULL | (1ULL << 63))
#define EFI_BUFFER_TOO_SMALL      (5ULL | (1ULL << 63))
#define EFI_NOT_READY             (6ULL | (1ULL << 63))
#define EFI_DEVICE_ERROR          (7ULL | (1ULL << 63))
#define EFI_WRITE_PROTECTED       (8ULL | (1ULL << 63))
#define EFI_OUT_OF_RESOURCES      (9ULL | (1ULL << 63))
#define EFI_VOLUME_CORRUPTED      (10ULL | (1ULL << 63))
#define EFI_VOLUME_FULL           (11ULL | (1ULL << 63))
#define EFI_NO_MEDIA              (12ULL | (1ULL << 63))
#define EFI_MEDIA_CHANGED         (13ULL | (1ULL << 63))
#define EFI_NOT_FOUND             (14ULL | (1ULL << 63))
#define EFI_ACCESS_DENIED         (15ULL | (1ULL << 63))
#define EFI_NO_RESPONSE           (16ULL | (1ULL << 63))
#define EFI_NO_MAPPING            (17ULL | (1ULL << 63))
#define EFI_TIMEOUT               (18ULL | (1ULL << 63))
#define EFI_NOT_STARTED           (19ULL | (1ULL << 63))
#define EFI_ALREADY_STARTED       (20ULL | (1ULL << 63))
#define EFI_ABORTED               (21ULL | (1ULL << 63))

#define EFI_ERROR(status)         (((INTN)(status)) < 0)

/* GUID structure */
typedef struct {
    UINT32 Data1;
    UINT16 Data2;
    UINT16 Data3;
    UINT8  Data4[8];
} EFI_GUID;

/* Standard GUIDs */
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    { 0x9042a9de, 0x23dc, 0x4a38, { 0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a } }

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    { 0x5B1B31A1, 0x9562, 0x11d2, { 0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B } }

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
    { 0x0964e5b22, 0x6459, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_FILE_INFO_ID \
    { 0x09576e92, 0x6d3f, 0x11d2, { 0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b } }

#define EFI_ACPI_20_TABLE_GUID \
    { 0x8868e871, 0xe4f1, 0x11d3, { 0xbc, 0x22, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

#define ACPI_10_TABLE_GUID \
    { 0xeb9d2d30, 0x2d88, 0x11d3, { 0x9a, 0x16, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d } }

/* Table Header */
typedef struct {
    UINT64 Signature;
    UINT32 Revision;
    UINT32 HeaderSize;
    UINT32 CRC32;
    UINT32 Reserved;
} EFI_TABLE_HEADER;

/* Input Protocol */
typedef struct {
    UINT16 ScanCode;
    CHAR16 UnicodeChar;
} EFI_INPUT_KEY;

#define SCAN_UP    0x0001
#define SCAN_DOWN  0x0002
#define SCAN_RIGHT 0x0003
#define SCAN_LEFT  0x0004
#define SCAN_ESC   0x0017

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    IN struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    IN BOOLEAN ExtendedVerification
);
typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    IN struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    OUT EFI_INPUT_KEY *Key
);

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
    EFI_INPUT_RESET     Reset;
    EFI_INPUT_READ_KEY  ReadKeyStroke;
    EFI_EVENT           WaitForKey;
} EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

/* Output Protocol */
struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_TEXT_RESET)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN ExtendedVerification
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_STRING)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16 *String
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_TEST_STRING)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16 *String
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_QUERY_MODE)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN ModeNumber,
    OUT UINTN *Columns,
    OUT UINTN *Rows
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_MODE)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN ModeNumber
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_ATTRIBUTE)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN Attribute
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_SET_CURSOR_POSITION)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN UINTN Column,
    IN UINTN Row
);
typedef EFI_STATUS (EFIAPI *EFI_TEXT_ENABLE_CURSOR)(
    IN struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN BOOLEAN Visible
);

typedef struct {
    INT32   MaxMode;
    INT32   Mode;
    INT32   Attribute;
    INT32   CursorColumn;
    INT32   CursorRow;
    BOOLEAN CursorVisible;
} SIMPLE_TEXT_OUTPUT_MODE;

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
    EFI_TEXT_RESET                Reset;
    EFI_TEXT_STRING               OutputString;
    EFI_TEXT_TEST_STRING          TestString;
    EFI_TEXT_QUERY_MODE           QueryMode;
    EFI_TEXT_SET_MODE             SetMode;
    EFI_TEXT_SET_ATTRIBUTE        SetAttribute;
    EFI_TEXT_CLEAR_SCREEN         ClearScreen;
    EFI_TEXT_SET_CURSOR_POSITION  SetCursorPosition;
    EFI_TEXT_ENABLE_CURSOR        EnableCursor;
    SIMPLE_TEXT_OUTPUT_MODE       *Mode;
} EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

/* Memory Types */
typedef enum {
    EfiReservedMemoryType,
    EfiLoaderCode,
    EfiLoaderData,
    EfiBootServicesCode,
    EfiBootServicesData,
    EfiRuntimeServicesCode,
    EfiRuntimeServicesData,
    EfiConventionalMemory,
    EfiUnusableMemory,
    EfiACPIReclaimMemory,
    EfiACPIMemoryNVS,
    EfiMemoryMappedIO,
    EfiMemoryMappedIOPortSpace,
    EfiPalCode,
    EfiPersistentMemory,
    EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef enum {
    AllocateAnyPages,
    AllocateMaxAddress,
    AllocateAddress,
    MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef struct {
    UINT32                Type;
    UINT32                Pad;
    EFI_PHYSICAL_ADDRESS  PhysicalStart;
    EFI_VIRTUAL_ADDRESS   VirtualStart;
    UINT64                NumberOfPages;
    UINT64                Attribute;
} EFI_MEMORY_DESCRIPTOR;

/* Graphics Output Protocol (GOP) */
typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,
    PixelBlueGreenRedReserved8BitPerColor,
    PixelBitMask,
    PixelBltOnly,
    PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
    UINT32 RedMask;
    UINT32 GreenMask;
    UINT32 BlueMask;
    UINT32 ReservedMask;
} EFI_PIXEL_BITMASK;

typedef struct {
    UINT32                     Version;
    UINT32                     HorizontalResolution;
    UINT32                     VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
    EFI_PIXEL_BITMASK          PixelInformation;
    UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
    UINT8 Blue;
    UINT8 Green;
    UINT8 Red;
    UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef enum {
    EfiBltVideoFill,
    EfiBltVideoToBltBuffer,
    EfiBltBufferToVideo,
    EfiBltVideoToVideo,
    EfiGraphicsOutputBltOperationMax
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

typedef struct {
    UINT32                                 MaxMode;
    UINT32                                 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
    UINTN                                  SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
    UINTN                                  FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    IN struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber,
    OUT UINTN *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION **Info
);
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    IN struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN UINT32 ModeNumber
);
typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT)(
    IN struct _EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
    IN OUT EFI_GRAPHICS_OUTPUT_BLT_PIXEL *BltBuffer, OPTIONAL
    IN EFI_GRAPHICS_OUTPUT_BLT_OPERATION BltOperation,
    IN UINTN SourceX,
    IN UINTN SourceY,
    IN UINTN DestinationX,
    IN UINTN DestinationY,
    IN UINTN Width,
    IN UINTN Height,
    IN UINTN Delta OPTIONAL
);

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
} EFI_GRAPHICS_OUTPUT_PROTOCOL;

/* File System Protocol */
struct _EFI_FILE_PROTOCOL;

#define EFI_FILE_MODE_READ   0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE  0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE 0x8000000000000000ULL

#define EFI_FILE_READ_ONLY   0x0000000000000001ULL
#define EFI_FILE_DIRECTORY   0x0000000000000010ULL

typedef struct {
    UINT64 Size;
    UINT64 FileSize;
    UINT64 PhysicalSize;
    UINT64 CreateTime;
    UINT64 LastAccessTime;
    UINT64 ModificationTime;
    UINT64 Attribute;
    CHAR16 FileName[1];
} EFI_FILE_INFO;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
    IN struct _EFI_FILE_PROTOCOL *This,
    OUT struct _EFI_FILE_PROTOCOL **NewHandle,
    IN CHAR16 *FileName,
    IN UINT64 OpenMode,
    IN UINT64 Attributes
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(
    IN struct _EFI_FILE_PROTOCOL *This
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_DELETE)(
    IN struct _EFI_FILE_PROTOCOL *This
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(
    IN struct _EFI_FILE_PROTOCOL *This,
    IN OUT UINTN *BufferSize,
    OUT VOID *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_WRITE)(
    IN struct _EFI_FILE_PROTOCOL *This,
    IN OUT UINTN *BufferSize,
    IN VOID *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(
    IN struct _EFI_FILE_PROTOCOL *This,
    IN UINT64 Position
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(
    IN struct _EFI_FILE_PROTOCOL *This,
    OUT UINT64 *Position
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_INFO)(
    IN struct _EFI_FILE_PROTOCOL *This,
    IN EFI_GUID *InformationType,
    IN OUT UINTN *BufferSize,
    OUT VOID *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_INFO)(
    IN struct _EFI_FILE_PROTOCOL *This,
    IN EFI_GUID *InformationType,
    IN UINTN BufferSize,
    IN VOID *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FILE_FLUSH)(
    IN struct _EFI_FILE_PROTOCOL *This
);

typedef struct _EFI_FILE_PROTOCOL {
    UINT64                Revision;
    EFI_FILE_OPEN         Open;
    EFI_FILE_CLOSE        Close;
    EFI_FILE_DELETE       Delete;
    EFI_FILE_READ         Read;
    EFI_FILE_WRITE        Write;
    EFI_FILE_GET_POSITION GetPosition;
    EFI_FILE_SET_POSITION SetPosition;
    EFI_FILE_GET_INFO     GetInfo;
    EFI_FILE_SET_INFO     SetInfo;
    EFI_FILE_FLUSH        Flush;
} EFI_FILE_PROTOCOL;

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;
typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
    IN struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    OUT EFI_FILE_PROTOCOL **Root
);

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
    UINT64                                       Revision;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME  OpenVolume;
} EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

/* Loaded Image Protocol */
typedef struct {
    UINT32           Revision;
    EFI_HANDLE       ParentHandle;
    VOID             *SystemTable;
    EFI_HANDLE       DeviceHandle;
    VOID             *FilePath;
    VOID             *Reserved;
    UINT32           ImageSize;
    EFI_PHYSICAL_ADDRESS ImageBase;
    EFI_PHYSICAL_ADDRESS ImageCodeType;
    EFI_PHYSICAL_ADDRESS ImageDataType;
    EFI_PHYSICAL_ADDRESS Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

/* Boot Services */
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_PAGES)(
    IN EFI_ALLOCATE_TYPE Type,
    IN EFI_MEMORY_TYPE MemoryType,
    IN UINTN Pages,
    IN OUT EFI_PHYSICAL_ADDRESS *Memory
);
typedef EFI_STATUS (EFIAPI *EFI_FREE_PAGES)(
    IN EFI_PHYSICAL_ADDRESS Memory,
    IN UINTN Pages
);
typedef EFI_STATUS (EFIAPI *EFI_GET_MEMORY_MAP)(
    IN OUT UINTN *MemoryMapSize,
    IN OUT EFI_MEMORY_DESCRIPTOR *MemoryMap,
    OUT UINTN *MapKey,
    OUT UINTN *DescriptorSize,
    OUT UINT32 *DescriptorVersion
);
typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    IN EFI_MEMORY_TYPE PoolType,
    IN UINTN Size,
    OUT VOID **Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(
    IN VOID *Buffer
);
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(
    IN UINTN NumberOfEvents,
    IN EFI_EVENT *Event,
    OUT UINTN *Index
);
typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    IN EFI_HANDLE Handle,
    IN EFI_GUID *Protocol,
    OUT VOID **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_LOCATE_PROTOCOL)(
    IN EFI_GUID *Protocol,
    IN VOID *Registration OPTIONAL,
    OUT VOID **Interface
);
typedef EFI_STATUS (EFIAPI *EFI_EXIT_BOOT_SERVICES)(
    IN EFI_HANDLE ImageHandle,
    IN UINTN MapKey
);
typedef EFI_STATUS (EFIAPI *EFI_SET_MEM)(
    IN VOID *Buffer,
    IN UINTN Size,
    IN UINT8 Value
);
typedef EFI_STATUS (EFIAPI *EFI_COPY_MEM)(
    IN VOID *Destination,
    IN VOID *Source,
    IN UINTN Length
);
typedef EFI_STATUS (EFIAPI *EFI_STALL)(
    IN UINTN Microseconds
);

typedef struct {
    EFI_TABLE_HEADER            Hdr;

    /* Task Priority Services */
    VOID*                       RaiseTPL;
    VOID*                       RestoreTPL;

    /* Memory Services */
    EFI_ALLOCATE_PAGES          AllocatePages;
    EFI_FREE_PAGES              FreePages;
    EFI_GET_MEMORY_MAP          GetMemoryMap;
    EFI_ALLOCATE_POOL           AllocatePool;
    EFI_FREE_POOL               FreePool;

    /* Event & Timer Services */
    VOID*                       CreateEvent;
    VOID*                       SetTimer;
    EFI_WAIT_FOR_EVENT          WaitForEvent;
    VOID*                       SignalEvent;
    VOID*                       CloseEvent;
    VOID*                       CheckEvent;

    /* Protocol Handler Services */
    VOID*                       InstallProtocolInterface;
    VOID*                       ReinstallProtocolInterface;
    VOID*                       UninstallProtocolInterface;
    EFI_HANDLE_PROTOCOL         HandleProtocol;
    VOID*                       Void;
    VOID*                       RegisterProtocolNotify;
    VOID*                       LocateHandle;
    VOID*                       LocateDevicePath;
    VOID*                       InstallConfigurationTable;

    /* Image Services */
    VOID*                       LoadImage;
    VOID*                       StartImage;
    VOID*                       Exit;
    VOID*                       UnloadImage;
    EFI_EXIT_BOOT_SERVICES      ExitBootServices;

    /* Misc Services */
    VOID*                       GetNextMonotonicCount;
    EFI_STALL                   Stall;
    VOID*                       SetWatchdogTimer;

    /* DriverSupport Services */
    VOID*                       ConnectController;
    VOID*                       DisconnectController;

    /* Open and Close Protocol Services */
    VOID*                       OpenProtocol;
    VOID*                       CloseProtocol;
    VOID*                       OpenProtocolInformation;

    /* Library Services */
    VOID*                       ProtocolsPerHandle;
    VOID*                       LocateHandleBuffer;
    EFI_LOCATE_PROTOCOL         LocateProtocol;
    VOID*                       InstallMultipleProtocolInterfaces;
    VOID*                       UninstallMultipleProtocolInterfaces;

    /* 32-bit CRC Services */
    VOID*                       CalculateCrc32;

    /* Misc Services (UEFI 1.1+) */
    EFI_COPY_MEM                CopyMem;
    EFI_SET_MEM                 SetMem;
    VOID*                       CreateEventEx;
} EFI_BOOT_SERVICES;

/* Runtime Services */
typedef enum {
    EfiResetCold,
    EfiResetWarm,
    EfiResetShutdown,
    EfiResetPlatformSpecific
} EFI_RESET_TYPE;

typedef VOID (EFIAPI *EFI_RESET_SYSTEM)(
    IN EFI_RESET_TYPE ResetType,
    IN EFI_STATUS ResetStatus,
    IN UINTN DataSize,
    IN VOID *ResetData OPTIONAL
);

typedef struct {
    EFI_TABLE_HEADER            Hdr;
    VOID*                       GetTime;
    VOID*                       SetTime;
    VOID*                       GetWakeupTime;
    VOID*                       SetWakeupTime;
    VOID*                       SetVirtualAddressMap;
    VOID*                       ConvertPointer;
    VOID*                       GetVariable;
    VOID*                       GetNextVariableName;
    VOID*                       SetVariable;
    VOID*                       GetNextHighMonotonicCount;
    EFI_RESET_SYSTEM            ResetSystem;
    VOID*                       UpdateCapsule;
    VOID*                       QueryCapsuleCapabilities;
    VOID*                       QueryVariableInfo;
} EFI_RUNTIME_SERVICES;

/* Configuration Table */
typedef struct {
    EFI_GUID  VendorGuid;
    VOID*     VendorTable;
} EFI_CONFIGURATION_TABLE;

/* System Table */
typedef struct {
    EFI_TABLE_HEADER                 Hdr;
    CHAR16                           *FirmwareVendor;
    UINT32                           FirmwareRevision;
    EFI_HANDLE                       ConsoleInHandle;
    EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *ConIn;
    EFI_HANDLE                       ConsoleOutHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;
    EFI_HANDLE                       StandardErrorHandle;
    EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *StdErr;
    EFI_RUNTIME_SERVICES             *RuntimeServices;
    EFI_BOOT_SERVICES                *BootServices;
    UINTN                            NumberOfTableEntries;
    EFI_CONFIGURATION_TABLE          *ConfigurationTable;
} EFI_SYSTEM_TABLE;

#endif /* _BOOTLOADER_EFI_H_ */
