/**
 * @file    lcd_ssd1306.c
 * @brief   Driver SSD1306 128×64 OLED via I2C — Detector de Mentiras
 *
 * Implementación completa sin dependencias externas (solo i2c_driver.h).
 * Incluye fuente 5×7, funciones de texto y barra de progreso.
 *
 * @author  Kheara Kieley, Mauricio Vela, Ximena Cedillo, Michelle Urbina
 */

#include "lcd_ssd1306.h"
#include "i2c_driver.h"
#include <stddef.h>

/* ── Acceso directo a GPIOB sin incluir stm32f4xx.h (evita redefinición
 * de macros I2C con CMSIS).  Solo necesitamos MODER, OTYPER, PUPDR, AFR. */
#define GPIOB_BASE_ADDR     0x40020400UL

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_Mini_t;

#define GPIOB_MINI  ((GPIO_Mini_t *)(GPIOB_BASE_ADDR))

/* RCC AHB1ENR para habilitar GPIOB */
#define RCC_AHB1ENR_ADDR    0x40023830UL
#define RCC_AHB1ENR         (*((volatile uint32_t *)(RCC_AHB1ENR_ADDR)))
#define RCC_AHB1ENR_GPIOBEN (1U << 1U)

/* Puntero I2C1 usando el tipo del driver (evita conflicto con CMSIS) */
#define MY_I2C1  I2C_PERIPH(I2C1_BASE_ADDR)

/* ═══════════════════════════════════════════════════════════════════════
 * CONSTANTES INTERNAS
 * ═══════════════════════════════════════════════════════════════════════ */

#define PAGES           8U          /* 128×64 → 8 páginas de 8 px      */
#define CMD_BYTE        0x00U       /* Co=0, D/C=0 → comando            */
#define DATA_BYTE       0x40U       /* Co=0, D/C=1 → dato               */

/* ── Secuencia de inicialización SSD1306 ─────────────────────────────── */
static const uint8_t k_init_seq[] = {
    0xAE,        /* Display OFF                  */
    0xD5, 0x80,  /* Clock divide / osc freq      */
    0xA8, 0x3F,  /* Multiplex ratio = 64         */
    0xD3, 0x00,  /* Display offset = 0           */
    0x40,        /* Start line = 0               */
    0x8D, 0x14,  /* Charge pump ON               */
    0x20, 0x00,  /* Horizontal addressing mode   */
    0xA1,        /* Segment remap (col 127→SEG0) */
    0xC8,        /* COM scan direction remapped  */
    0xDA, 0x12,  /* COM pins hardware config     */
    0x81, 0xCF,  /* Contrast = 207               */
    0xD9, 0xF1,  /* Pre-charge period            */
    0xDB, 0x40,  /* VCOMH deselect level         */
    0xA4,        /* Entire display ON (RAM)      */
    0xA6,        /* Normal display (not inverted)*/
    0xAF,        /* Display ON                   */
};

/* ═══════════════════════════════════════════════════════════════════════
 * FUENTE 5×7 — ASCII 32–127
 * Cada carácter = 5 bytes, un bit por pixel, MSB arriba
 * ═══════════════════════════════════════════════════════════════════════ */
static const uint8_t k_font5x7[][5] = {
    {0x00,0x00,0x00,0x00,0x00}, /* ' ' 32 */
    {0x00,0x00,0x5F,0x00,0x00}, /* '!' 33 */
    {0x00,0x07,0x00,0x07,0x00}, /* '"' 34 */
    {0x14,0x7F,0x14,0x7F,0x14}, /* '#' 35 */
    {0x24,0x2A,0x7F,0x2A,0x12}, /* '$' 36 */
    {0x23,0x13,0x08,0x64,0x62}, /* '%' 37 */
    {0x36,0x49,0x55,0x22,0x50}, /* '&' 38 */
    {0x00,0x05,0x03,0x00,0x00}, /* ''' 39 */
    {0x00,0x1C,0x22,0x41,0x00}, /* '(' 40 */
    {0x00,0x41,0x22,0x1C,0x00}, /* ')' 41 */
    {0x14,0x08,0x3E,0x08,0x14}, /* '*' 42 */
    {0x08,0x08,0x3E,0x08,0x08}, /* '+' 43 */
    {0x00,0x50,0x30,0x00,0x00}, /* ',' 44 */
    {0x08,0x08,0x08,0x08,0x08}, /* '-' 45 */
    {0x00,0x60,0x60,0x00,0x00}, /* '.' 46 */
    {0x20,0x10,0x08,0x04,0x02}, /* '/' 47 */
    {0x3E,0x51,0x49,0x45,0x3E}, /* '0' 48 */
    {0x00,0x42,0x7F,0x40,0x00}, /* '1' 49 */
    {0x42,0x61,0x51,0x49,0x46}, /* '2' 50 */
    {0x21,0x41,0x45,0x4B,0x31}, /* '3' 51 */
    {0x18,0x14,0x12,0x7F,0x10}, /* '4' 52 */
    {0x27,0x45,0x45,0x45,0x39}, /* '5' 53 */
    {0x3C,0x4A,0x49,0x49,0x30}, /* '6' 54 */
    {0x01,0x71,0x09,0x05,0x03}, /* '7' 55 */
    {0x36,0x49,0x49,0x49,0x36}, /* '8' 56 */
    {0x06,0x49,0x49,0x29,0x1E}, /* '9' 57 */
    {0x00,0x36,0x36,0x00,0x00}, /* ':' 58 */
    {0x00,0x56,0x36,0x00,0x00}, /* ';' 59 */
    {0x08,0x14,0x22,0x41,0x00}, /* '<' 60 */
    {0x14,0x14,0x14,0x14,0x14}, /* '=' 61 */
    {0x00,0x41,0x22,0x14,0x08}, /* '>' 62 */
    {0x02,0x01,0x51,0x09,0x06}, /* '?' 63 */
    {0x32,0x49,0x79,0x41,0x3E}, /* '@' 64 */
    {0x7E,0x11,0x11,0x11,0x7E}, /* 'A' 65 */
    {0x7F,0x49,0x49,0x49,0x36}, /* 'B' 66 */
    {0x3E,0x41,0x41,0x41,0x22}, /* 'C' 67 */
    {0x7F,0x41,0x41,0x22,0x1C}, /* 'D' 68 */
    {0x7F,0x49,0x49,0x49,0x41}, /* 'E' 69 */
    {0x7F,0x09,0x09,0x09,0x01}, /* 'F' 70 */
    {0x3E,0x41,0x49,0x49,0x7A}, /* 'G' 71 */
    {0x7F,0x08,0x08,0x08,0x7F}, /* 'H' 72 */
    {0x00,0x41,0x7F,0x41,0x00}, /* 'I' 73 */
    {0x20,0x40,0x41,0x3F,0x01}, /* 'J' 74 */
    {0x7F,0x08,0x14,0x22,0x41}, /* 'K' 75 */
    {0x7F,0x40,0x40,0x40,0x40}, /* 'L' 76 */
    {0x7F,0x02,0x0C,0x02,0x7F}, /* 'M' 77 */
    {0x7F,0x04,0x08,0x10,0x7F}, /* 'N' 78 */
    {0x3E,0x41,0x41,0x41,0x3E}, /* 'O' 79 */
    {0x7F,0x09,0x09,0x09,0x06}, /* 'P' 80 */
    {0x3E,0x41,0x51,0x21,0x5E}, /* 'Q' 81 */
    {0x7F,0x09,0x19,0x29,0x46}, /* 'R' 82 */
    {0x46,0x49,0x49,0x49,0x31}, /* 'S' 83 */
    {0x01,0x01,0x7F,0x01,0x01}, /* 'T' 84 */
    {0x3F,0x40,0x40,0x40,0x3F}, /* 'U' 85 */
    {0x1F,0x20,0x40,0x20,0x1F}, /* 'V' 86 */
    {0x3F,0x40,0x38,0x40,0x3F}, /* 'W' 87 */
    {0x63,0x14,0x08,0x14,0x63}, /* 'X' 88 */
    {0x07,0x08,0x70,0x08,0x07}, /* 'Y' 89 */
    {0x61,0x51,0x49,0x45,0x43}, /* 'Z' 90 */
    {0x00,0x7F,0x41,0x41,0x00}, /* '[' 91 */
    {0x02,0x04,0x08,0x10,0x20}, /* '\' 92 */
    {0x00,0x41,0x41,0x7F,0x00}, /* ']' 93 */
    {0x04,0x02,0x01,0x02,0x04}, /* '^' 94 */
    {0x40,0x40,0x40,0x40,0x40}, /* '_' 95 */
    {0x00,0x01,0x02,0x04,0x00}, /* '`' 96 */
    {0x20,0x54,0x54,0x54,0x78}, /* 'a' 97 */
    {0x7F,0x48,0x44,0x44,0x38}, /* 'b' 98 */
    {0x38,0x44,0x44,0x44,0x20}, /* 'c' 99 */
    {0x38,0x44,0x44,0x48,0x7F}, /* 'd' 100 */
    {0x38,0x54,0x54,0x54,0x18}, /* 'e' 101 */
    {0x08,0x7E,0x09,0x01,0x02}, /* 'f' 102 */
    {0x0C,0x52,0x52,0x52,0x3E}, /* 'g' 103 */
    {0x7F,0x08,0x04,0x04,0x78}, /* 'h' 104 */
    {0x00,0x44,0x7D,0x40,0x00}, /* 'i' 105 */
    {0x20,0x40,0x44,0x3D,0x00}, /* 'j' 106 */
    {0x7F,0x10,0x28,0x44,0x00}, /* 'k' 107 */
    {0x00,0x41,0x7F,0x40,0x00}, /* 'l' 108 */
    {0x7C,0x04,0x18,0x04,0x78}, /* 'm' 109 */
    {0x7C,0x08,0x04,0x04,0x78}, /* 'n' 110 */
    {0x38,0x44,0x44,0x44,0x38}, /* 'o' 111 */
    {0x7C,0x14,0x14,0x14,0x08}, /* 'p' 112 */
    {0x08,0x14,0x14,0x18,0x7C}, /* 'q' 113 */
    {0x7C,0x08,0x04,0x04,0x08}, /* 'r' 114 */
    {0x48,0x54,0x54,0x54,0x20}, /* 's' 115 */
    {0x04,0x3F,0x44,0x40,0x20}, /* 't' 116 */
    {0x3C,0x40,0x40,0x40,0x7C}, /* 'u' 117 */
    {0x1C,0x20,0x40,0x20,0x1C}, /* 'v' 118 */
    {0x3C,0x40,0x30,0x40,0x3C}, /* 'w' 119 */
    {0x44,0x28,0x10,0x28,0x44}, /* 'x' 120 */
    {0x0C,0x50,0x50,0x50,0x3C}, /* 'y' 121 */
    {0x44,0x64,0x54,0x4C,0x44}, /* 'z' 122 */
    {0x00,0x08,0x36,0x41,0x00}, /* '{' 123 */
    {0x00,0x00,0x7F,0x00,0x00}, /* '|' 124 */
    {0x00,0x41,0x36,0x08,0x00}, /* '}' 125 */
    {0x08,0x08,0x2A,0x1C,0x08}, /* '~' 126 */
    {0x08,0x1C,0x2A,0x08,0x08}, /* DEL 127 */
};

/* ═══════════════════════════════════════════════════════════════════════
 * FRAME BUFFER
 * 128 columnas × 8 páginas = 1024 bytes
 * ═══════════════════════════════════════════════════════════════════════ */
static uint8_t s_buf[SSD1306_WIDTH * PAGES];

/* ═══════════════════════════════════════════════════════════════════════
 * HELPERS INTERNOS
 * ═══════════════════════════════════════════════════════════════════════ */

/** Envía un byte de comando al SSD1306 */
static void ssd_cmd(uint8_t cmd)
{
    uint8_t buf[2] = { CMD_BYTE, cmd };
    i2c_writeDevice(MY_I2C1, SSD1306_I2C_ADDR, buf, 2U);
}

/** Vuelca el framebuffer completo al display */
static void ssd_flush(void)
{
    /* Configurar ventana de escritura: toda la pantalla */
    ssd_cmd(0x21); ssd_cmd(0x00); ssd_cmd(0x7F); /* col 0–127 */
    ssd_cmd(0x22); ssd_cmd(0x00); ssd_cmd(0x07); /* page 0–7  */

    /* El SSD1306 espera: 0x40 seguido de los datos.
     * Enviamos en bloques de 16 bytes para no saturar el bus. */
    uint8_t pkt[17];
    pkt[0] = DATA_BYTE;
    for (uint16_t i = 0U; i < sizeof(s_buf); i += 16U)
    {
        for (uint8_t j = 0U; j < 16U; j++)
        {
            pkt[j + 1U] = s_buf[i + j];
        }
        i2c_writeDevice(MY_I2C1, SSD1306_I2C_ADDR, pkt, 17U);
    }
}

/** Limpia el framebuffer (todo negro) */
static void ssd_clear(void)
{
    for (uint16_t i = 0U; i < sizeof(s_buf); i++)
    {
        s_buf[i] = 0x00U;
    }
}

/** Enciende un pixel en (x, y) */
static void ssd_pixel(uint8_t x, uint8_t y)
{
    if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) { return; }
    s_buf[(uint16_t)x + (uint16_t)(y / 8U) * SSD1306_WIDTH] |=
        (uint8_t)(1U << (y % 8U));
}

/** Dibuja un carácter 5×7 en (x, page).
 *  page: 0–7 (cada página = 8 px de alto)
 *  Retorna la x siguiente disponible. */
static uint8_t ssd_char(uint8_t x, uint8_t page, char c)
{
    if (c < 32 || c > 127) { c = '?'; }
    const uint8_t *glyph = k_font5x7[(uint8_t)c - 32U];
    uint16_t base = (uint16_t)x + (uint16_t)page * SSD1306_WIDTH;
    for (uint8_t col = 0U; col < 5U; col++)
    {
        if ((x + col) < SSD1306_WIDTH)
        {
            s_buf[base + col] = glyph[col];
        }
    }
    /* 1 columna de espacio entre letras */
    if ((x + 5U) < SSD1306_WIDTH) { s_buf[base + 5U] = 0x00U; }
    return (uint8_t)(x + 6U);
}

/** Dibuja una cadena en (x, page). */
static void ssd_str(uint8_t x, uint8_t page, const char *str)
{
    while (*str && x < SSD1306_WIDTH)
    {
        x = ssd_char(x, page, *str);
        str++;
    }
}

/** Convierte uint32 a string decimal sin sprintf.
 *  buf debe tener al menos 11 bytes. Retorna puntero al inicio. */
static char *u32_to_str(uint32_t val, char *buf, uint8_t size)
{
    buf[size - 1U] = '\0';
    uint8_t i = size - 2U;
    if (val == 0U) { buf[i--] = '0'; }
    else {
        while (val > 0U && i > 0U) {
            buf[i--] = (char)('0' + (val % 10U));
            val /= 10U;
        }
        i++;
    }
    return &buf[i];
}

/** Dibuja barra de progreso horizontal.
 *  x0, y0: esquina superior izquierda; w: ancho total; pct: 0–100 */
static void ssd_bar(uint8_t x0, uint8_t y0, uint8_t w, uint8_t pct)
{
    uint8_t fill = (uint8_t)((uint16_t)w * pct / 100U);
    for (uint8_t x = x0; x < (uint8_t)(x0 + w); x++)
    {
        /* Borde superior e inferior */
        ssd_pixel(x, y0);
        ssd_pixel(x, (uint8_t)(y0 + 7U));
    }
    /* Relleno */
    for (uint8_t x = x0; x < (uint8_t)(x0 + fill); x++)
    {
        for (uint8_t y = (uint8_t)(y0 + 1U); y <= (uint8_t)(y0 + 6U); y++)
        {
            ssd_pixel(x, y);
        }
    }
}

/* ═══════════════════════════════════════════════════════════════════════
 * INICIALIZACIÓN I2C + SSD1306
 * ═══════════════════════════════════════════════════════════════════════ */

void lcd_init(void)
{
    /* ── PB8 (SCL) y PB9 (SDA) → AF4 (I2C1) ────────────────────────── */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    /* PB8: MODER[17:16] = 10 (AF), AFR[1][3:0] = 0100 (AF4) */
    GPIOB_MINI->MODER  &= ~(0x3U << 16U);
    GPIOB_MINI->MODER  |=  (0x2U << 16U);
    GPIOB_MINI->AFR[1] &= ~(0xFU << 0U);
    GPIOB_MINI->AFR[1] |=  (0x4U << 0U);
    /* PB9: MODER[19:18] = 10 (AF), AFR[1][7:4] = 0100 (AF4) */
    GPIOB_MINI->MODER  &= ~(0x3U << 18U);
    GPIOB_MINI->MODER  |=  (0x2U << 18U);
    GPIOB_MINI->AFR[1] &= ~(0xFU << 4U);
    GPIOB_MINI->AFR[1] |=  (0x4U << 4U);

    /* Open-drain + pull-up en PB8 y PB9 */
    GPIOB_MINI->OTYPER |=  (1U << 8U) | (1U << 9U);
    GPIOB_MINI->PUPDR  &= ~((0x3U << 16U) | (0x3U << 18U));
    GPIOB_MINI->PUPDR  |=  (0x1U << 16U)  | (0x1U << 18U);

    /* ── Inicializar I2C1 a 400 kHz ──────────────────────────────────── */
    i2c_config_t cfg = {
        .instance  = MY_I2C1,
        .clk_speed = I2C_SPEED_FM_HZ,
        .addr_mode = I2C_ADDR_7BIT,
    };
    i2c_init(&cfg);

    /* ── Secuencia de init del SSD1306 ───────────────────────────────── */
    for (uint8_t i = 0U; i < (uint8_t)sizeof(k_init_seq); i++)
    {
        ssd_cmd(k_init_seq[i]);
    }

    ssd_clear();
    ssd_flush();
}

/* ═══════════════════════════════════════════════════════════════════════
 * ACTUALIZAR PANTALLA
 * ═══════════════════════════════════════════════════════════════════════
 *
 * Layout 128×64:
 *
 *  Page 0-1  (px 0–15):   "LieP: XXX%"  (fuente normal)
 *  Page 2-3  (px 16–31):  Barra de progreso LieP
 *  Page 4-5  (px 32–47):  Estado (NORMAL / SOSPECHA / MENTIRA / SIN_CAL)
 *  Page 6    (px 48–55):  "BPM: XXX"
 *  Page 7    (px 56–63):  "R: XXX kΩ"
 * ═══════════════════════════════════════════════════════════════════════ */

void lcd_update(uint8_t lie_pct, const char *estado,
                uint16_t bpm, uint32_t r_skin)
{
    char tmp[12];
    char *p;

    ssd_clear();

    /* ── Página 0: Etiqueta "LieP:" ──────────────────────────────────── */
    ssd_str(0U, 0U, "LieP:");

    /* ── Página 1: Valor numérico del porcentaje ─────────────────────── */
    if (lie_pct == 255U)
    {
        ssd_str(0U, 1U, "---  SIN CAL");
    }
    else
    {
        p = u32_to_str((uint32_t)lie_pct, tmp, sizeof(tmp));
        ssd_str(0U, 1U, p);
        ssd_str((uint8_t)(6U * (uint8_t)(p - tmp + (uint8_t)(tmp[0]!= 0 ? 1:0)) + 12U),
                1U, "%");
    }

    /* ── Páginas 2–3: Barra de progreso (px 16–31) ───────────────────── */
    if (lie_pct != 255U)
    {
        ssd_bar(0U, 16U, 120U, lie_pct);
    }

    /* ── Páginas 4–5: Estado ─────────────────────────────────────────── */
    if (estado != NULL)
    {
        ssd_str(0U, 4U, estado);
    }

    /* ── Página 6: BPM ───────────────────────────────────────────────── */
    ssd_str(0U, 6U, "BPM:");
    if (bpm > 0U)
    {
        p = u32_to_str((uint32_t)bpm, tmp, sizeof(tmp));
        ssd_str(30U, 6U, p);
    }
    else
    {
        ssd_str(30U, 6U, "--");
    }

    /* ── Página 7: R_piel ────────────────────────────────────────────── */
    ssd_str(0U, 7U, "R:");
    if (r_skin >= 1000000UL)
    {
        /* Mostrar en MΩ con 1 decimal: r_skin / 100000 → X.X MΩ */
        uint32_t megas  = r_skin / 1000000UL;
        uint32_t dec    = (r_skin % 1000000UL) / 100000UL;
        p = u32_to_str(megas, tmp, sizeof(tmp));
        ssd_str(14U, 7U, p);
        ssd_str(14U + (uint8_t)(6U * 2U), 7U, ".");
        p = u32_to_str(dec, tmp, sizeof(tmp));
        ssd_str(14U + (uint8_t)(6U * 3U), 7U, p);
        ssd_str(14U + (uint8_t)(6U * 4U), 7U, "MO");  /* MΩ en ASCII */
    }
    else if (r_skin >= 1000UL)
    {
        uint32_t kilos = r_skin / 1000UL;
        p = u32_to_str(kilos, tmp, sizeof(tmp));
        ssd_str(14U, 7U, p);
        ssd_str(14U + (uint8_t)(6U * 3U), 7U, "kO");  /* kΩ en ASCII */
    }
    else
    {
        p = u32_to_str(r_skin, tmp, sizeof(tmp));
        ssd_str(14U, 7U, p);
        ssd_str(14U + (uint8_t)(6U * 4U), 7U, "O");
    }

    ssd_flush();
}

/* ═══════════════════════════════════════════════════════════════════════
 * GRÁFICA GSR + VALOR DE RESISTENCIA
 * ═══════════════════════════════════════════════════════════════════════
 *
 * Layout 128×64:
 *  Page 0   (px 0–7):    Etiqueta "GSR  R:XXXkO"
 *  px 8:                 Línea separadora horizontal
 *  Pages 1–7 (px 9–63):  Gráfica de barras verticales del historial GSR
 *
 * Historial circular de 128 muestras (1 columna = 1 muestra).
 * El valor ADC filtrado (0–4095) se mapea al área gráfica (55 px alto).
 * ═══════════════════════════════════════════════════════════════════════ */

#define GSR_GRAPH_SAMPLES   128U   /* columnas = ancho del display       */
#define GSR_GRAPH_HEIGHT    55U    /* px disponibles (px 9–63)           */
#define GSR_ADC_MAX         4095U  /* fondo de escala ADC 12-bit         */

static uint16_t s_gsr_history[GSR_GRAPH_SAMPLES];
static uint8_t  s_gsr_head  = 0U;
static uint8_t  s_gsr_count = 0U;

void lcd_update_gsr_graph(uint16_t gsr_filt, uint32_t r_skin)
{
    char tmp[12];
    char *p;

    /* ── Guardar muestra en historial circular ───────────────────────── */
    s_gsr_history[s_gsr_head] = gsr_filt;
    s_gsr_head = (uint8_t)((s_gsr_head + 1U) % GSR_GRAPH_SAMPLES);
    if (s_gsr_count < GSR_GRAPH_SAMPLES) { s_gsr_count++; }

    ssd_clear();

    /* ── Página 0: "GSR  R:XXXkO" ───────────────────────────────────── */
    ssd_str(0U, 0U, "GSR  R:");
    if (r_skin >= 1000000UL)
    {
        uint32_t megas = r_skin / 1000000UL;
        uint32_t dec   = (r_skin % 1000000UL) / 100000UL;
        uint8_t xp = 43U;
        p = u32_to_str(megas, tmp, sizeof(tmp));
        ssd_str(xp, 0U, p); xp = (uint8_t)(xp + 6U);
        ssd_str(xp, 0U, "."); xp = (uint8_t)(xp + 6U);
        p = u32_to_str(dec,   tmp, sizeof(tmp));
        ssd_str(xp, 0U, p); xp = (uint8_t)(xp + 6U);
        ssd_str(xp, 0U, "MO");
    }
    else if (r_skin >= 1000UL)
    {
        p = u32_to_str(r_skin / 1000UL, tmp, sizeof(tmp));
        ssd_str(43U, 0U, p);
        ssd_str(61U, 0U, "kO");
    }
    else
    {
        p = u32_to_str(r_skin, tmp, sizeof(tmp));
        ssd_str(43U, 0U, p);
        ssd_str(67U, 0U, "O");
    }

    /* ── Línea separadora en y=8 ─────────────────────────────────────── */
    for (uint8_t x = 0U; x < SSD1306_WIDTH; x++) { ssd_pixel(x, 8U); }

    /* ── Gráfica de barras (px 9–63) ─────────────────────────────────── */
    uint8_t n = (s_gsr_count < GSR_GRAPH_SAMPLES)
                ? s_gsr_count : (uint8_t)GSR_GRAPH_SAMPLES;

    for (uint8_t i = 0U; i < n; i++)
    {
        uint8_t buf_idx = (s_gsr_count < GSR_GRAPH_SAMPLES)
                          ? i
                          : (uint8_t)((s_gsr_head + i) % GSR_GRAPH_SAMPLES);

        uint16_t val   = s_gsr_history[buf_idx];
        uint8_t  bar_h = (uint8_t)(((uint32_t)val * GSR_GRAPH_HEIGHT)
                                   / GSR_ADC_MAX);
        if (bar_h == 0U) { bar_h = 1U; }

        uint8_t col   = (uint8_t)(SSD1306_WIDTH - n + i);
        uint8_t y_top = (uint8_t)(63U - bar_h + 1U);
        if (y_top < 9U) { y_top = 9U; }

        for (uint8_t y = y_top; y <= 63U; y++) { ssd_pixel(col, y); }
    }

    ssd_flush();
}
