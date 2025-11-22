#include <lcom/lcf.h>
#include <string.h>
#include "graphics.h"
#include <math.h>
#include "font.h"

vbe_mode_info_t mode_info;
uint8_t* frame_buffer;
uint8_t* back_buffer;
size_t buffer_size;

int (init_back_buffer)() {
  // Inicializa o buffer de vídeo
    size_t bytes_per_pixel = (mode_info.BitsPerPixel + 7) / 8;
    buffer_size = mode_info.XResolution * mode_info.YResolution * bytes_per_pixel;

    back_buffer = malloc(buffer_size);
    if (back_buffer == NULL) {
        printf("Error allocating back buffer\n");
        return 1;
    }

    memset(back_buffer, 0, buffer_size); // Limpa o buffer com zeros
    return 0;
}

// Mudança do Minix para modo gráfico
int (set_graphic_mode)(uint16_t submode) {
    reg86_t reg86;
    memset(&reg86, 0, sizeof(reg86)); // inicialização da estrutura com o valor 0 em todos os parâmetros
    reg86.intno = 0x10;               // intno é sempre 0x10      
    reg86.ah = 0x4F;                  // parte mais significativa de AX
    reg86.al = 0x02;                  // parte menos significativa de AX. 0x02 no caso de modo gráfico
    // reg86.ax = 0x4F02;             // equivamente às duas últimas instruções
    reg86.bx = submode | BIT(14);     // determinação do submodo com memória linear
    if (sys_int86(&reg86) != 0) {     // se houver algum erro, abortar a função
        printf("Set graphic mode failed\n");
        return 1;
    }
    return 0;
}

// Mudança do Minix para modo de texto
// Implementação interna da função vg_exit() já dada pela LCF
int (set_text_mode)() {
    reg86_t reg86;                       
    memset(&reg86, 0, sizeof(reg86));   // inicialização da estrutura com o valor 0 em todos os parâmetros
    reg86.intno = 0x10;                 // intno é sempre 0x10 
    reg86.ah = 0x00;                    // parte mais significativa de AX 
    reg86.al = 0x03;                    // parte menos significativa de AX. 0x03 no caso de modo texto
    // reg86.ax = 0x0003;               // equivamente às duas últimas instruções
    reg86.bx = 0x0000;                  // não há submodo no modo de texto
    if(sys_int86(&reg86) != 0) {        // se houver algum erro, abortar a função
        printf("Set text mode failed\n");
        return 1;
    }
    return 0;
}

// Construção do frame buffer virtual e físico
int (set_frame_buffer)(uint16_t mode){

  // retirar informação sobre o @mode
  memset(&mode_info, 0, sizeof(mode_info));
  if(vbe_get_mode_info(mode, &mode_info)) return 1;

  // cálculo do tamanho do frame buffer, em bytes
  unsigned int bytes_per_pixel = (mode_info.BitsPerPixel + 7) / 8;
  unsigned int frame_size = mode_info.XResolution * mode_info.YResolution * bytes_per_pixel;
  
  // preenchimento dos endereços físicos
  struct minix_mem_range physic_addresses;
  physic_addresses.mr_base = mode_info.PhysBasePtr; // início físico do buffer
  physic_addresses.mr_limit = physic_addresses.mr_base + frame_size; // fim físico do buffer
  
  // alocação física da memória necessária para o frame buffer
  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &physic_addresses)) {
    printf("Physical memory allocation error\n");
    return 1;
  }

  // alocação virtual da memória necessária para o frame buffer
  frame_buffer = vm_map_phys(SELF, (void*) physic_addresses.mr_base, frame_size);
  if (frame_buffer == NULL) {
    printf("Virtual memory allocation error\n");
    return 1;
  }

  return 0;
}

// Atualização da cor de um pixel
int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {

  // As coordenadas têm de ser válidas
  if(x >= mode_info.XResolution || y >= mode_info.YResolution) return 1;
  
  // Cálculo dos Bytes per pixel da cor escolhida. Arredondamento por excesso.
  unsigned BytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;

  // Índice (em bytes) da zona do píxel a colorir
  unsigned int index = (mode_info.XResolution * y + x) * BytesPerPixel;

  // A partir da zona frame_buffer[index], copia @BytesPerPixel bytes da @color
  if (memcpy(&frame_buffer[index], &color, BytesPerPixel) == NULL) return 1;
  // Se o back buffer estiver ativo, também atualiza o back buffer
  return 0;
}

// Desenha uma linha horizontal
int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (unsigned i = 0 ; i < len ; i++)   
    if (vg_draw_pixel(x+i, y, color) != 0) return 1;
  return 0;
}

// Desenha um rectângulo
int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for(unsigned i = 0; i < height ; i++)
    if (vg_draw_hline(x, y+i, width, color) != 0) {
      return 1;
    }
  return 0;
}

// Imprime uma imagem no formato XPM
int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {

  xpm_image_t img;

  // retorna um apontador para um array de cores preenchido de acordo com o XPM
  uint8_t *colors = xpm_load(xpm, XPM_INDEXED, &img);
  if (colors == NULL) return 1;

  for (int h = 0 ; h < img.height ; h++) {
    for (int w = 0 ; w < img.width ; w++) {
      if (vg_draw_pixel(x + w, y + h, *colors) != 0) return 1;
      colors++; // avança para a próxima cor
    }
  }
  return 0;
}

//from rgb to 565
uint32_t (rgb_to_565)(uint32_t color) {
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Desenha um caracter na posição (x, y) com a cor @color
int (draw_char)(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale) {
    if (c < 32 || c > 127) return 1; // unsupported char
    const uint8_t *glyph = font8x8_basic[c - 32];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (1 << col)) {
                // Draw a scale x scale block for each pixel
                for (int dx = 0; dx < scale; dx++) {
                    for (int dy = 0; dy < scale; dy++) {
                        vg_draw_pixel(x + col * scale + dx, y + row * scale + dy, color);
                    }
                }
            }
        }
    }
    return 0;
}

// Desenha uma string na posição (x, y) com a cor @color
int (draw_string)(const char *str, uint16_t x, uint16_t y, uint32_t color, uint8_t scale) {
    while (*str) {
        draw_char(*str, x, y, color, scale);
        x += 8 * scale; // Move to next character position
        str++;
    }
    return 0;
}

// Desenha um pixel no back buffer
int draw_pixel_to_backbuffer(uint16_t x, uint16_t y, uint32_t color) {
    if (x >= mode_info.XResolution || y >= mode_info.YResolution) return 1;

    unsigned BytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;

    size_t index = (y * mode_info.XResolution + x) * BytesPerPixel;

    if (memcpy(&back_buffer[index], &color, BytesPerPixel) == NULL) {
        printf("Error writing to back buffer\n");
        return 1;
    }
    return 0;
}
// Desenha uma string no back buffer
int draw_string_backbuffer(const char *str, int x, int y, uint32_t color, uint8_t scale) {
    for (const char *p = str; *p != '\0'; p++) {
        unsigned char c = (unsigned char)(*p);
        if (c < 32 || c > 127) c = '?';
        const uint8_t *glyph = font8x8_basic[c - 32];
        // Draw glyph row by row
        for (int row = 0; row < 8; row++) {
            uint8_t bits = glyph[row];
            for (int col = 0; col < 8; col++) {
                if (bits & (1 << col)) {
                    // draw scaled pixel block
                    for (int sy = 0; sy < scale; sy++) {
                        for (int sx = 0; sx < scale; sx++) {
                            if (draw_pixel_to_backbuffer(x + col*scale + sx,
                                                     y + row*scale + sy,
                                                     color) != 0) {
                                printf("Error drawing pixel to back buffer\n");
                                return 1;
                            } 
                        }
                    }
                }
            }
        }
        x += 8 * scale;
    }
    return 0;
}

// Em modos de cores com bytes incompletos (por exemplo 0x110 - 5:5:5) os bits extra
// são colocados a 0. A máscara usada é constuída pelo número de bits por pixel do modo.
// Exemplo:
// Modo 0x110 -> BitsPerPixel = 15
// Máscara = BIT(15) - 1 = 0b1000000000000000 - 1 = 0b0111111111111111
int normalize_color(uint32_t color, uint32_t *new_color) {
  if (mode_info.BitsPerPixel == 32) {
    *new_color = color;
  } else {
    *new_color = color & (BIT(mode_info.BitsPerPixel) - 1);
  }
  return 0;
}

// Funções auxiliares da video_test_pattern()

uint32_t (direct_mode)(uint32_t R, uint32_t G, uint32_t B) {
  return (R << mode_info.RedFieldPosition) | (G << mode_info.GreenFieldPosition) | (B << mode_info.BlueFieldPosition);
}

uint32_t (indexed_mode)(uint16_t col, uint16_t row, uint8_t step, uint32_t first, uint8_t n) {
  return (first + (row * n + col) * step) % (1 << mode_info.BitsPerPixel);
}

uint32_t (Red)(unsigned j, uint8_t step, uint32_t first) {
  return (R(first) + j * step) % (1 << mode_info.RedMaskSize);
}

uint32_t (Green)(unsigned i, uint8_t step, uint32_t first) {
  return (G(first) + i * step) % (1 << mode_info.GreenMaskSize);
}

uint32_t (Blue)(unsigned j, unsigned i, uint8_t step, uint32_t first) {
  return (B(first) + (i + j) * step) % (1 << mode_info.BlueMaskSize);
}

uint32_t (R)(uint32_t first){
  return ((1 << mode_info.RedMaskSize) - 1) & (first >> mode_info.RedFieldPosition);
}

uint32_t (G)(uint32_t first){
  return ((1 << mode_info.GreenMaskSize) - 1) & (first >> mode_info.GreenFieldPosition);
}

uint32_t (B)(uint32_t first){
  return ((1 << mode_info.BlueMaskSize) - 1) & (first >> mode_info.BlueFieldPosition);
}
