MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  .fixed_ram 0x20003000 (NOLOAD) :
  {
    KEEP(*(.fixed_ram))
  } > RAM

  .telem_ram 0x20003000 (NOLOAD) :
  {
    KEEP(*(.telem_ram))
  } > RAM
}
