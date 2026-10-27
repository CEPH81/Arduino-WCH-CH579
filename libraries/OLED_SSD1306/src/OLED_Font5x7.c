/********************************** UTILIDAD *************************************
* File Name          : OLED_Font5x7.c (antes Font5x7.c)
* Description        : Fuente bitmap 5x7 (5 columnas x 7 filas por caracter),
*                       el formato clasico usado en displays OLED/LCD graficos
*                       de bajo costo. Cubre A-Z, 0-9, espacio y puntuacion
*                       basica. Agrega mas casos al switch para ampliarla.
**********************************************************************************/

#include "OLED_Font5x7.h"

static const uint8_t F_SPACE[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

static const uint8_t F_A[5] = { 0x7E, 0x11, 0x11, 0x11, 0x7E };
static const uint8_t F_B[5] = { 0x7F, 0x49, 0x49, 0x49, 0x36 };
static const uint8_t F_C[5] = { 0x3E, 0x41, 0x41, 0x41, 0x22 };
static const uint8_t F_D[5] = { 0x7F, 0x41, 0x41, 0x22, 0x1C };
static const uint8_t F_E[5] = { 0x7F, 0x49, 0x49, 0x49, 0x41 };
static const uint8_t F_F[5] = { 0x7F, 0x09, 0x09, 0x09, 0x01 };
static const uint8_t F_G[5] = { 0x3E, 0x41, 0x49, 0x49, 0x7A };
static const uint8_t F_H[5] = { 0x7F, 0x08, 0x08, 0x08, 0x7F };
static const uint8_t F_I[5] = { 0x00, 0x41, 0x7F, 0x41, 0x00 };
static const uint8_t F_J[5] = { 0x20, 0x40, 0x41, 0x3F, 0x01 };
static const uint8_t F_K[5] = { 0x7F, 0x08, 0x14, 0x22, 0x41 };
static const uint8_t F_L[5] = { 0x7F, 0x40, 0x40, 0x40, 0x40 };
static const uint8_t F_M[5] = { 0x7F, 0x02, 0x0C, 0x02, 0x7F };
static const uint8_t F_N[5] = { 0x7F, 0x04, 0x08, 0x10, 0x7F };
static const uint8_t F_O[5] = { 0x3E, 0x41, 0x41, 0x41, 0x3E };
static const uint8_t F_P[5] = { 0x7F, 0x09, 0x09, 0x09, 0x06 };
static const uint8_t F_Q[5] = { 0x3E, 0x41, 0x51, 0x21, 0x5E };
static const uint8_t F_R[5] = { 0x7F, 0x09, 0x19, 0x29, 0x46 };
static const uint8_t F_S[5] = { 0x46, 0x49, 0x49, 0x49, 0x31 };
static const uint8_t F_T[5] = { 0x01, 0x01, 0x7F, 0x01, 0x01 };
static const uint8_t F_U[5] = { 0x3F, 0x40, 0x40, 0x40, 0x3F };
static const uint8_t F_V[5] = { 0x1F, 0x20, 0x40, 0x20, 0x1F };
static const uint8_t F_W[5] = { 0x3F, 0x40, 0x38, 0x40, 0x3F };
static const uint8_t F_X[5] = { 0x63, 0x14, 0x08, 0x14, 0x63 };
static const uint8_t F_Y[5] = { 0x07, 0x08, 0x70, 0x08, 0x07 };
static const uint8_t F_Z[5] = { 0x61, 0x51, 0x49, 0x45, 0x43 };

static const uint8_t F_0[5] = { 0x3E, 0x51, 0x49, 0x45, 0x3E };
static const uint8_t F_1[5] = { 0x00, 0x42, 0x7F, 0x40, 0x00 };
static const uint8_t F_2[5] = { 0x42, 0x61, 0x51, 0x49, 0x46 };
static const uint8_t F_3[5] = { 0x22, 0x41, 0x49, 0x49, 0x36 };
static const uint8_t F_4[5] = { 0x18, 0x14, 0x12, 0x7F, 0x10 };
static const uint8_t F_5[5] = { 0x27, 0x45, 0x45, 0x45, 0x39 };
static const uint8_t F_6[5] = { 0x3C, 0x4A, 0x49, 0x49, 0x30 };
static const uint8_t F_7[5] = { 0x01, 0x71, 0x09, 0x05, 0x03 };
static const uint8_t F_8[5] = { 0x36, 0x49, 0x49, 0x49, 0x36 };
static const uint8_t F_9[5] = { 0x26, 0x49, 0x49, 0x49, 0x3E };

static const uint8_t F_DOT[5]    = { 0x00, 0x60, 0x60, 0x00, 0x00 };
static const uint8_t F_COMMA[5]  = { 0x00, 0x80, 0x60, 0x00, 0x00 };
static const uint8_t F_COLON[5]  = { 0x00, 0x36, 0x36, 0x00, 0x00 };
static const uint8_t F_EXCL[5]   = { 0x00, 0x00, 0x5F, 0x00, 0x00 };
static const uint8_t F_QUEST[5]  = { 0x02, 0x01, 0x51, 0x09, 0x06 };
static const uint8_t F_DASH[5]   = { 0x08, 0x08, 0x08, 0x08, 0x08 };
static const uint8_t F_SLASH[5]  = { 0x20, 0x10, 0x08, 0x04, 0x02 };
static const uint8_t F_EQUAL[5]  = { 0x14, 0x14, 0x14, 0x14, 0x14 };
static const uint8_t F_PLUS[5]   = { 0x08, 0x08, 0x3E, 0x08, 0x08 };

uint8_t Font5x7_GetGlyph(char c, const uint8_t **glyph)
{
    /* Letras minusculas se tratan como mayusculas (fuente solo mayusculas) */
    if (c >= 'a' && c <= 'z')
    {
        c = (char)(c - 'a' + 'A');
    }

    switch (c)
    {
        case 'A': *glyph = F_A; break;
        case 'B': *glyph = F_B; break;
        case 'C': *glyph = F_C; break;
        case 'D': *glyph = F_D; break;
        case 'E': *glyph = F_E; break;
        case 'F': *glyph = F_F; break;
        case 'G': *glyph = F_G; break;
        case 'H': *glyph = F_H; break;
        case 'I': *glyph = F_I; break;
        case 'J': *glyph = F_J; break;
        case 'K': *glyph = F_K; break;
        case 'L': *glyph = F_L; break;
        case 'M': *glyph = F_M; break;
        case 'N': *glyph = F_N; break;
        case 'O': *glyph = F_O; break;
        case 'P': *glyph = F_P; break;
        case 'Q': *glyph = F_Q; break;
        case 'R': *glyph = F_R; break;
        case 'S': *glyph = F_S; break;
        case 'T': *glyph = F_T; break;
        case 'U': *glyph = F_U; break;
        case 'V': *glyph = F_V; break;
        case 'W': *glyph = F_W; break;
        case 'X': *glyph = F_X; break;
        case 'Y': *glyph = F_Y; break;
        case 'Z': *glyph = F_Z; break;

        case '0': *glyph = F_0; break;
        case '1': *glyph = F_1; break;
        case '2': *glyph = F_2; break;
        case '3': *glyph = F_3; break;
        case '4': *glyph = F_4; break;
        case '5': *glyph = F_5; break;
        case '6': *glyph = F_6; break;
        case '7': *glyph = F_7; break;
        case '8': *glyph = F_8; break;
        case '9': *glyph = F_9; break;

        case '.': *glyph = F_DOT;   break;
        case ',': *glyph = F_COMMA; break;
        case ':': *glyph = F_COLON; break;
        case '!': *glyph = F_EXCL;  break;
        case '?': *glyph = F_QUEST; break;
        case '-': *glyph = F_DASH;  break;
        case '/': *glyph = F_SLASH; break;
        case '=': *glyph = F_EQUAL; break;
        case '+': *glyph = F_PLUS;  break;

        case ' ':
        default:  *glyph = F_SPACE; break;
    }

    return 5;
}
