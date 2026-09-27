#include <stdint.h>

void kernel_main(void)
{
    volatile uint16_t *video = (uint16_t *)0xB8000;

    video[0] = 0x0F00 | '7';
    video[1] = 0x0F00 | '2';
    video[2] = 0x0F00 | '5';
    video[3] = 0x0F00 | 'O';
    video[4] = 0x0F00 | 'S';

    while (1)
    {
        __asm__ volatile("hlt");
    }
}