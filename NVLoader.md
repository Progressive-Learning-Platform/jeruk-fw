# envy #

`envy` is the JERUK non-volatile memory / flash loader. The code resides  in [envy.c](https://code.google.com/p/jeruk-fw/source/browse/testfw.X/envy.c) and it loads itself into RAM before executing so all of the flash memory can be written to (once I figure out how to load the loader to RAM). Flash programming for the PIC32 microcontroller family is described in Section 5 of the PIC32 Family Reference Manual ([Microchip doc# 61121F](http://ww1.microchip.com/downloads/en/DeviceDoc/61121F.pdf)). Latest releases of the xc32 compiler only provide pre-compiled libraries for the nvm peripheral code, but all the pertinent routines to allow programming to the flash memory are listed and described in this document.

**Notes on PIC32 NVM Operations**

All addresses must be **physical**. Convert the virtual addresses of the flash region to program and the data buffer in RAM by masking it with `0x1fffffff` before loading these addresses to the NVM\*ADDR registers. Otherwise WRERR will be set because you will be trying to flash unmapped memory region. WRERR is **not** cleared with a soft reset. This error bit prevents further erase/write operations. POR (Power-on Reset) will clear it, or run an NVM no-op (nvmop=0x4000). There is currently a terminal command '`envycl`' to clear WRERR.

Different devices have different page and row sizes! PIC32MX270F256B and all PIC32MX1XX/2XX family processors have **1KB page size** and **128 bytes row size**. The corresponding masks for these sizes are `0x3FF` and `0x7F`. This information is from the [PIC32MX1XX/2XX Family manual, section 5.0](http://ww1.microchip.com/downloads/en/DeviceDoc/60001168F.pdf).

# Principles of Operation #

`envy` can currently be invoked by sending the '`envy`' command to the terminal. `envy` provides a UART1 interface for the PIC32 Flash programming routines. The following is the packet structure for programming the flash memory through envy:

| 'n' or 0x6e | base address ID (1-byte) | size in bytes (2-bytes, 0-64KB) | byte stream with size specified in the previous field, broken into pages |
|:------------|:-------------------------|:--------------------------------|:-------------------------------------------------------------------------|

envy will buffer 1KB (PIC32MX1XX/2XX) pages and return the character 'v' or 0x76 after a page has been written to flash. The transmitting host **must wait** after every 1KB of data until the 'v' character has been received before resuming the byte stream. Otherwise envy will miss your data because it is busy erasing the page and then writing the data to the flash memory page.

After the correct number of bytes have been received, envy will pad the rest of the page with 1s if the bytestream ends before a page boundary. envy will then send a 1-byte checksum (sum of every byte in the stream). This checksum is calculated by reading the  data written to the flash memory **up to the specified size**, i.e. the 0xff padding is not included in checksum calculation. If envy failed to erase or write a 1KB page to flash, it will return the character 'e' or 0x65 and halt.

envy divides the flash memory into 64KB segments. The base address ID is used to calculate the base address of the incoming program (e.g. ID of 1 will make envy to start to program from address `0x9d010000`). ID of 0 is where the bootloader resides and writing to this segment will overwrite the bootloader (self-update). **Note:** envy is still being executed off program flash memory (PFM), so writing to segment 0 will probably overwrite it. If this happens, you will have to reflash the processor with a programmer.

The following example will write 0xde 0xad 0xbe 0xef to the beginning of slot 1 (0x9d010000):

| 'n' | 0x01 | 0x00 0x04 (big endian) | 0xde 0xad 0xbe 0xef |
|:----|:-----|:-----------------------|:--------------------|

Flash memory 0x9d010000 will contain the following after a successful operation (with the rest of the 1KB page filled with 0xff):

> `0x9d010000  de ad be ef  ff ff ff ff  ff ff ff ff  ff ff ff ff`

Use the following packet to jump to a segment:

| 'j' or 0x6a | base address ID (1-byte) |
|:------------|:-------------------------|

# envy Routines #

`unsigned int NVMUnlock(unsigned int nvmop)`

> Perform the specified NVM operation. Refer to the Flash programming specification for details on the unlocking of NVM feature and specific NVM ops. Use this routine with caution. Returns the WRERR (0x2000) and LVDERR (0x1000) bits. Returns 0 if operation was successful.

`unsigned int envy_write_word(void* address, unsigned int data)`

> Write a word of data (32-bit) to the specified address. The address can be virtual or physical. Returns WRERR/LVDERR (0 on successful operation). Note: write can only clear the bits, erase the page first to set the whole page to 1s.

`unsigned int envy_write_row(void* address, void* data)`

> Write a row of data starting from the specified address (virtual/physical). A row on the PIC32MX1xx/2xx family is 128 bytes. Returns WRERR/LVDERR (0 on successful operation). Note: write can only clear the bits, erase the page first to set the whole page to 1s.

`unsigned int envy_erase_page(void* address)`

> Erase the page specified by the address. Make sure that the address is page aligned (1KB or 0x400 for PIC32MX1xx/2xx). A Flash memory page needs to be erased first before it is written. I.e. if the user only wants to write to a specific word or row, the user must buffer the whole page, erase, make the change on the buffer, then write back the buffer to the page. Returns WRERR/LVDERR (0 on successful operation).

`void envy_write_stream(unsigned int base_offset, unsigned int size)`

> See envy packet structure above. Read a byte stream off UART1 and write to flash memory from base address of flash memory + `base_offset` up to the specified size in bytes. This routine buffers up to each page boundary before performing an erase/write cycle for the page. The routine will send back 'v' if the write is successful. It will halt otherwise (erase or write error). `base_offset` must be page-aligned. `size` does not have to be page-aligned (envy will pad 1s to page boundary).

`void envy()`

> Invoke envy.