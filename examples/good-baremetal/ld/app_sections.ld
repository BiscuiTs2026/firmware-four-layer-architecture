/* 片段示例：固定地址快照段。整份链接脚本由工程提供，这里只展示相关段落。 */

MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
}

SECTIONS
{
  /* 蓝牙链路统计块：0x20003000 .. 0x20003037（56 字节） */
  .fixed_ram 0x20003000 (NOLOAD) :
  {
    KEEP(*(.fixed_ram))
  } > RAM

  /* 遥测快照：0x20003900 .. 0x2000390F（16 字节），与上一段不重叠 */
  .telem_ram 0x20003900 (NOLOAD) :
  {
    KEEP(*(.telem_ram))
  } > RAM
}
