/*
 * QEMU lm3s6965evb Cortex-M3 보드용 linker script입니다.
 *
 * linker script는 컴파일된 코드와 데이터를 어떤 메모리 주소에 배치할지 정합니다.
 * Cortex-M firmware에서는 vector table이 FLASH 시작 위치에 있어야 합니다.
 */

MEMORY
{
  /* 실행 코드와 읽기 전용 데이터가 들어가는 영역입니다. */
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K

  /* 실행 중 바뀌는 전역 변수, heap, stack이 들어가는 영역입니다. */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

/* stack은 RAM 끝에서 아래 방향으로 자랍니다. */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  /*
   * vector table은 startup.c의 vector_table 배열입니다.
   * KEEP은 linker가 "사용하지 않는 섹션"으로 판단해 제거하지 못하게 합니다.
   */
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  /*
   * 실행 코드와 문자열 상수 같은 읽기 전용 데이터를 FLASH에 배치합니다.
   */
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  /*
   * .data는 실행 중 RAM에 있어야 하지만, 초기값은 FLASH에 저장됩니다.
   * Reset_Handler가 _sidata에서 _sdata로 값을 복사합니다.
   */
  _sidata = LOADADDR(.data);

  .data :
  {
    _sdata = .;
    *(.data*)
    _edata = .;
  } > RAM AT > FLASH

  /*
   * .bss는 0으로 초기화되는 전역/static 변수 영역입니다.
   * Reset_Handler가 _sbss부터 _ebss까지 0으로 채웁니다.
   */
  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > RAM
}
