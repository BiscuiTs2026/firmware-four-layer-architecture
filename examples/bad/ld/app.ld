/*
 * 违规 12：两个固定地址段落在同一地址，且都没标 (NOLOAD)
 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  .snap_ram 0x20003800 :
  {
    KEEP(*(.snap_ram))
  } >RAM

  .log_ram 0x20003800 :
  {
    KEEP(*(.log_ram))
  } >RAM
}
