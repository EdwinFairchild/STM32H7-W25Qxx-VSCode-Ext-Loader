# STM32 External Loader (GCC) using W25Q64

This project leverages the STM32VSCode extension using CMake to build an external loader for the STM32H723 microcontroller, since STs external-loader repo only has IAR based projects. This project utilizes the OSPI peripheral in QSPI mode to interface with the external Winbond W25Q64 Serial NOR flash memory chip.

This can easily be adapted to other serial QSPI memories since they tend to use the same standard set of command. Do your research, as well as other ST MCUs using SPI,QSPI,OSPI etc. 
### Tips
- An external loader file  is nothing more than the .elf file with a new .stldr extension. Just rename the file.
- The `StorageInfo` struct must be place in its own section of the linker script

```c
struct StorageInfo __attribute__((section(".Dev_info"))) /*const*/ StorageInfo = {
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
```
- The `StorageInfo` struct is expected by STM32CubeProgrammer to be exactly 200 bytes (0xc8). If you adjust the `SECTOR_NUM` macro to less than 10 you will have to add padding bytes at the end of the struct to meet the 200 byte size.
```c
#define SECTOR_NUM 10               // Max Number of Sector types

struct DeviceSectors {
    unsigned long     SectorNum;     // Number of Sectors
    unsigned long     SectorSize;    // Sector Size in Bytes
};

struct StorageInfo {
    char              DeviceName[100];           // Device Name and Description
    unsigned short DeviceType;                   // Device Type: ONCHIP, EXT8BIT, EXT16BIT, ...
    unsigned long  DeviceStartAddress;       // Default Device Start Address
    unsigned long  DeviceSize;                   // Total Size of Device
    unsigned long  PageSize;                 // Programming Page Size
    unsigned char  EraseValue;                   // Content of Erased Memory
    struct     DeviceSectors  sectors[SECTOR_NUM];
};
```
- Generally want to avoid anything that uses interrupts such as `HAL_Delay` which relies on `SysTick`. Redefine delay related functions, these are defined as weak in HAL so no additional step is required to redefine them in the `Loader_Src.c` file.
```c
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
    // silence compiler about unused parameter
    (void)TickPriority;
    return HAL_OK;
}

uint32_t HAL_GetTick(void)
{
    return 1;
}

void HAL_Delay(uint32_t delay_ms)
{
    volatile uint32_t dummy = 0;
    // 40000  was calcualted on a 200 MHz System clockassuming about 5 cycles per loop iteration
    uint32_t iterations = 40000 * delay_ms; // Adjust for crude delay
    for (uint32_t i = 0; i < iterations; i++)
    {
        dummy++;
    }
}
```
### The linker script
- Your entry point is no longer `main`. The new entry point which STM32CubeProgrammer will expect is `Init`
- The entirety of the operations run in ram, the linker will only consist of RAM memory. 
- The linker script is segmented into two sections, `Loader` and `SgInfo`
- Device info section is loaded into SgInfo
```c
    .Dev_Info :
  {
      __Dev_info_START = .;
      *(.Dev_info*)
      KEEP(*(.Dev_info))
      __Dev_info_END = .;
  }  >RAM_D1 :SgInfo
  ```
  - Layout of the memory in this script should be maintained, STM32CubeProgrammer is very finnicky 
  - Modify default generated linker flags in `cmake/gcc-arm-none-eabi.cmake` to include unused sections
  ```c
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/stm32h723zgtx_flash.ld\"")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map,${CMAKE_PROJECT_NAME}.map")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -lc -lm")
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -lstdc++ -lsupc++")
  ```