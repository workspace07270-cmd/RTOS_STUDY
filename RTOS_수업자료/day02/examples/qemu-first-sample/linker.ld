/*
 * QEMU lm3s6965evb 보드에서 사용할 단순 linker script입니다.
 *
 * linker script는 컴파일된 코드와 데이터를 어느 메모리 주소에 배치할지 결정합니다.
 * Cortex-M firmware에서는 vector table이 반드시 시작 주소 근처에 있어야 합니다.
 */

MEMORY
{
  /*
   * FLASH:
   *   코드와 읽기 전용 데이터가 들어가는 영역입니다.
   *   QEMU 샘플에서는 0x00000000부터 시작한다고 가정합니다.
   */
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 256K

  /*
   * RAM:
   *   실행 중 변경되는 데이터, bss, stack이 들어가는 영역입니다.
   *   Cortex-M에서 SRAM은 보통 0x20000000 근처에 매핑됩니다.
   */
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

/*
 * stack은 RAM 끝에서 아래 방향으로 자라는 구조가 일반적입니다.
 * _estack은 startup.c의 vector table 첫 번째 값으로 사용됩니다.
 */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  /*
   * vector table은 firmware 시작 지점에 있어야 합니다.
   * startup.c에서 .isr_vector 섹션으로 지정한 배열이 여기에 배치됩니다.
   */
  .isr_vector :
  {
    KEEP(*(.isr_vector))
  } > FLASH

  /*
   * 실행 코드와 문자열 상수 같은 읽기 전용 데이터입니다.
   */
  .text :
  {
    *(.text*)
    *(.rodata*)
  } > FLASH

  /*
   * 초기값이 있는 전역/static 데이터 영역입니다.
   * 이 첫 샘플에서는 거의 사용하지 않지만 firmware 구조 설명을 위해 둡니다.
   */
  .data :
  {
    *(.data*)
  } > RAM AT > FLASH

  /*
   * 초기값이 0인 전역/static 데이터 영역입니다.
   */
  .bss :
  {
    *(.bss*)
    *(COMMON)
  } > RAM
}
