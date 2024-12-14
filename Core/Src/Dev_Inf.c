/*
 * Dev_Inf.c
 *
 */
#include "Dev_Inf.h"
#include "octospi.h"
#include "w25q64.h"

/* This structure contains information used by ST-LINK Utility to program and erase the device */
#if defined(__ICCARM__)
__root struct StorageInfo const StorageInfo = {
#else
struct StorageInfo __attribute__((section(".Dev_info"))) /*const*/ StorageInfo = {
#endif
    "w25q64",        // Device Name + version number
    NOR_FLASH,       // Device Type
    0x90000000,      // Device Start Address
    W25Q_FLASH_SIZE, // Device Size in Bytes
    W25Q_PAGE_SIZE,  // Programming Page Size
    0xFF,            // Initial Content of Erased Memory

    // Specify Size and Address of Sectors (view example below)
    {{
         (W25Q_FLASH_SIZE/W25Q_SECTOR_SIZE),  // Sector Numbers,
         W25Q_SECTOR_SIZE // Sector Size
     },

     {0x00000000, 0x00000000}}};
