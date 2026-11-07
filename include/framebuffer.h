#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H
#include <stdint.h>
#include <stddef.h>

#define DISPLAY_WIDTH        128
#define DISPLAY_HEIGHT       64
#define FONT_WIDTH           5
#define DISPLAY_BUFFER_SIZE  ((DISPLAY_WIDTH * DISPLAY_HEIGHT) / 8)

uint8_t *FrameBuffer_GetBuffer(void);
size_t   FrameBuffer_GetSize(void);

void FrameBuffer_Clear();
void FrameBuffer_WriteChar(uint8_t *frame_buffer, char ascii_char, uint8_t x, uint8_t page);
void FrameBuffer_WriteString(uint8_t *frame_buffer, char *str, uint8_t x, uint8_t page);

#endif