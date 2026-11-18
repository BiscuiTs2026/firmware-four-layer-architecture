/* 对抗夹具：固定地址段 */
MEMORY
{
  RAM (xrw) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
  /* 区间重叠：a 占 0x20003000..0x2000303F，b 从 0x20003020 开始，起始地址不同 */
  .gap_a 0x20003000 (NOLOAD) :
  {
    /* size: 0x40 */
    KEEP(*(.gap_a))
  } > RAM

  .gap_b 0x20003020 (NOLOAD) :
  {
    /* size: 0x40 */
    KEEP(*(.gap_b))
  } > RAM

  /* 历史事故复现：通配符把后续自定义段一起吞掉 */
  .gap_c 0x20004000 (NOLOAD) :
  {
    KEEP(*(.gap_c*))
  } > RAM
}
