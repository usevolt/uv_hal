/* -------------------------------------------------------------------------------- */
/* -- �GUI - Generic GUI module (C)Achim D�bler, 2015                            -- */
/* -------------------------------------------------------------------------------- */
// �GUI is a generic GUI module for embedded systems.
// This is a free software that is open for education, research and commercial
// developments under license policy of following terms.
//
//  Copyright (C) 2015, Achim D�bler, all rights reserved.
//  URL: http://www.embeddedlightning.com/
//
// * The �GUI module is a free software and there is NO WARRANTY.
// * No restriction on use. You can use, modify and redistribute it for
//   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
// * Redistributions of source code must retain the above copyright notice.
//
/* -------------------------------------------------------------------------------- */
/* -- REVISION HISTORY                                                           -- */
/* -------------------------------------------------------------------------------- */
//  Mar 18, 2015  V0.3  Driver support added.
//                      Window and object support added.
//                      Touch support added.
//                      Fixed some minor bugs.
//
//  Oct 20, 2014  V0.2  Function UG_DrawRoundFrame() added.
//                      Function UG_FillRoundFrame() added.
//                      Function UG_DrawArc() added.
//                      Fixed some minor bugs.
//
//  Oct 11, 2014  V0.1  First release.
/* -------------------------------------------------------------------------------- */


//#include "system.h"
#include <stdint.h>
#include <uv_hal_config.h>

#ifndef __UGUI_H
#define __UGUI_H


/* -------------------------------------------------------------------------------- */
/* -- CONFIG SECTION                                                             -- */
/* -------------------------------------------------------------------------------- */



/* Specify platform-dependent integer types here */

#define __UG_CONST   const
typedef uint8_t      UG_U8;
typedef int8_t       UG_S8;
typedef uint16_t     UG_U16;
typedef int16_t      UG_S16;
typedef uint32_t     UG_U32;
typedef int32_t      UG_S32;


/* Example for dsPIC33
typedef unsigned char         UG_U8;
typedef signed char           UG_S8;
typedef unsigned int          UG_U16;
typedef signed int            UG_S16;
typedef unsigned long int     UG_U32;
typedef signed long int       UG_S32;
*/

/* -------------------------------------------------------------------------------- */
/* -------------------------------------------------------------------------------- */




/* -------------------------------------------------------------------------------- */
/* -- �GUI FONTS                                                                 -- */
/* -- Source: http://www.mikrocontroller.net/user/show/benedikt                  -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
   unsigned char* p;
   UG_S16 char_width;
   UG_S16 char_height;
} UG_FONT;



#if CONFIG_UI_FONT_4X6
   extern const UG_FONT FONT_4X6;
#endif
#if CONFIG_UI_FONT_5X8
   extern const UG_FONT FONT_5X8;
#endif
#if CONFIG_UI_FONT_5X12
   extern const UG_FONT FONT_5X12;
#endif
#if CONFIG_UI_FONT_6X8
   extern const UG_FONT FONT_6X8;
#endif
#if CONFIG_UI_FONT_6X10
   extern const UG_FONT FONT_6X10;
#endif
#if CONFIG_UI_FONT_7X12
   extern const UG_FONT FONT_7X12;
#endif
#if CONFIG_UI_FONT_8X8
   extern const UG_FONT FONT_8X8;
#endif
#if CONFIG_UI_FONT_8X12
   extern const UG_FONT FONT_8X12;
#endif
#if CONFIG_UI_FONT_8X14
   extern const UG_FONT FONT_8X14;
#endif
#if CONFIG_UI_FONT_10X16
   extern const UG_FONT FONT_10X16;
#endif
#if CONFIG_UI_FONT_12X16
   extern const UG_FONT FONT_12X16;
#endif
#if CONFIG_UI_FONT_12X20
   extern const UG_FONT FONT_12X20;
#endif
#if CONFIG_UI_FONT_16X26
   extern const UG_FONT FONT_16X26;
#endif
#if CONFIG_UI_FONT_22X36
   extern const UG_FONT FONT_22X36;
#endif
#if CONFIG_UI_FONT_24X40
   extern const UG_FONT FONT_24X40;
#endif
#if CONFIG_UI_FONT_32X53
   extern const UG_FONT FONT_32X53;
#endif

/* -------------------------------------------------------------------------------- */
/* -- TYPEDEFS                                                                   -- */
/* -------------------------------------------------------------------------------- */
typedef struct S_OBJECT                               UG_OBJECT;
typedef struct S_WINDOW                               UG_WINDOW;
typedef UG_S8                                         UG_RESULT;
typedef UG_U32                                        UG_COLOR;
/* -------------------------------------------------------------------------------- */
/* -- DEFINES                                                                    -- */
/* -------------------------------------------------------------------------------- */
#ifndef NULL
   #define NULL ((void*) 0)
#endif

/* Alignments */
#define ALIGN_H_LEFT                                  (1<<0)
#define ALIGN_H_CENTER                                (1<<1)
#define ALIGN_H_RIGHT                                 (1<<2)
#define ALIGN_V_TOP                                   (1<<3)
#define ALIGN_V_CENTER                                (1<<4)
#define ALIGN_V_BOTTOM                                (1<<5)
#define ALIGN_BOTTOM_RIGHT                            (ALIGN_V_BOTTOM|ALIGN_H_RIGHT)
#define ALIGN_BOTTOM_CENTER                           (ALIGN_V_BOTTOM|ALIGN_H_CENTER)
#define ALIGN_BOTTOM_LEFT                             (ALIGN_V_BOTTOM|ALIGN_H_LEFT)
#define ALIGN_CENTER_RIGHT                            (ALIGN_V_CENTER|ALIGN_H_RIGHT)
#define ALIGN_CENTER                                  (ALIGN_V_CENTER|ALIGN_H_CENTER)
#define ALIGN_CENTER_LEFT                             (ALIGN_V_CENTER|ALIGN_H_LEFT)
#define ALIGN_TOP_RIGHT                               (ALIGN_V_TOP|ALIGN_H_RIGHT)
#define ALIGN_TOP_CENTER                              (ALIGN_V_TOP|ALIGN_H_CENTER)
#define ALIGN_TOP_LEFT                                (ALIGN_V_TOP|ALIGN_H_LEFT)

/* Default IDs */
#define OBJ_ID_0                                      0
#define OBJ_ID_1                                      1
#define OBJ_ID_2                                      2
#define OBJ_ID_3                                      3
#define OBJ_ID_4                                      4
#define OBJ_ID_5                                      5
#define OBJ_ID_6                                      6
#define OBJ_ID_7                                      7
#define OBJ_ID_8                                      8
#define OBJ_ID_9                                      9
#define OBJ_ID_10                                     10
#define OBJ_ID_11                                     11
#define OBJ_ID_12                                     12
#define OBJ_ID_13                                     13
#define OBJ_ID_14                                     14
#define OBJ_ID_15                                     15
#define OBJ_ID_16                                     16
#define OBJ_ID_17                                     17
#define OBJ_ID_18                                     18
#define OBJ_ID_19                                     19

/* -------------------------------------------------------------------------------- */
/* -- FUNCTION RESULTS                                                           -- */
/* -------------------------------------------------------------------------------- */
#define UG_RESULT_FAIL                               -1
#define UG_RESULT_OK                                  0

/* -------------------------------------------------------------------------------- */
/* -- UNIVERSAL STRUCTURES                                                       -- */
/* -------------------------------------------------------------------------------- */
/* Area structure */
typedef struct
{
   UG_S16 xs;
   UG_S16 ys;
   UG_S16 xe;
   UG_S16 ye;
} UG_AREA;

/* Text structure */
typedef struct
{
   char* str;
   const UG_FONT* font;
   UG_AREA a;
   UG_COLOR fc;
   UG_COLOR bc;
   UG_U8 align;
   UG_S16 h_space;
   UG_S16 v_space;
} UG_TEXT;

/* -------------------------------------------------------------------------------- */
/* -- BITMAP                                                                     -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
   void* p;
   UG_U16 width;
   UG_U16 height;
   UG_U8 bpp;
   UG_U8 colors;
} UG_BMP;

#define BMP_BPP_1                                     (1<<0)
#define BMP_BPP_2                                     (1<<1)
#define BMP_BPP_4                                     (1<<2)
#define BMP_BPP_8                                     (1<<3)
#define BMP_BPP_16                                    (1<<4)
#define BMP_BPP_32                                    (1<<5)
#define BMP_RGB888                                    (1<<0)
#define BMP_RGB565                                    (1<<1)
#define BMP_RGB555                                    (1<<2)

/* -------------------------------------------------------------------------------- */
/* -- MESSAGE                                                                    -- */
/* -------------------------------------------------------------------------------- */
/* Message structure */
typedef struct
{
   UG_U8 type;
   UG_U8 id;
   UG_U8 sub_id;
   UG_U8 event;
   void* src;
} UG_MESSAGE;

/* Message types */
#define MSG_TYPE_NONE                                 0
#define MSG_TYPE_WINDOW                               1
#define MSG_TYPE_OBJECT                               2

/* -------------------------------------------------------------------------------- */
/* -- TOUCH                                                                      -- */
/* -------------------------------------------------------------------------------- */
/* Touch structure */
typedef struct
{
   UG_U8 state;
   UG_S16 xp;
   UG_S16 yp;
} UG_TOUCH;


#define TOUCH_STATE_PRESSED                           1
#define TOUCH_STATE_RELEASED                          0

/* -------------------------------------------------------------------------------- */
/* -- OBJECTS                                                                    -- */
/* -------------------------------------------------------------------------------- */
/* Object structure */
struct S_OBJECT
{
   UG_U8 state;                              /* object state                               */
   UG_U8 touch_state;                        /* object touch state                         */
   void (*update) (UG_WINDOW*,UG_OBJECT*);   /* pointer to object-specific update function */
   UG_AREA a_abs;                            /* absolute area of the object                */
   UG_AREA a_rel;                            /* relative area of the object                */
   UG_U8 type;                               /* object type                                */
   UG_U8 id;                                 /* object ID                                  */
   UG_U8 event;                              /* object-specific events                     */
   void* data;                               /* pointer to object-specific data            */
};

/* Currently supported objects */
#define OBJ_TYPE_NONE                                 0
#define OBJ_TYPE_BUTTON                               1
#define OBJ_TYPE_TEXTBOX                              2
#define OBJ_TYPE_IMAGE                                3

/* Standard object events */
#define OBJ_EVENT_NONE                                0
#define OBJ_EVENT_CLICKED                             1

/* Object states */
#define OBJ_STATE_FREE                                (1<<0)
#define OBJ_STATE_VALID                               (1<<1)
#define OBJ_STATE_BUSY                                (1<<2)
#define OBJ_STATE_VISIBLE                             (1<<3)
#define OBJ_STATE_ENABLE                              (1<<4)
#define OBJ_STATE_UPDATE                              (1<<5)
#define OBJ_STATE_REDRAW                              (1<<6)
#define OBJ_STATE_TOUCH_ENABLE                        (1<<7)
#define OBJ_STATE_INIT                                (OBJ_STATE_FREE | OBJ_STATE_VALID)

/* Object touch states */
#define OBJ_TOUCH_STATE_CHANGED                       (1<<0)
#define OBJ_TOUCH_STATE_PRESSED_ON_OBJECT             (1<<1)
#define OBJ_TOUCH_STATE_PRESSED_OUTSIDE_OBJECT        (1<<2)
#define OBJ_TOUCH_STATE_RELEASED_ON_OBJECT            (1<<3)
#define OBJ_TOUCH_STATE_RELEASED_OUTSIDE_OBJECT       (1<<4)
#define OBJ_TOUCH_STATE_IS_PRESSED_ON_OBJECT          (1<<5)
#define OBJ_TOUCH_STATE_IS_PRESSED                    (1<<6)
#define OBJ_TOUCH_STATE_CLICK_ON_OBJECT               (1<<7)
#define OBJ_TOUCH_STATE_INIT                          0

/* -------------------------------------------------------------------------------- */
/* -- WINDOW                                                                     -- */
/* -------------------------------------------------------------------------------- */
/* Title structure */
typedef struct
{
   char* str;
   const UG_FONT* font;
   UG_S8 h_space;
   UG_S8 v_space;
   UG_U8 align;
   UG_COLOR fc;
   UG_COLOR bc;
   UG_COLOR ifc;
   UG_COLOR ibc;
   UG_U8 height;
} UG_TITLE;

/* Window structure */
struct S_WINDOW
{
   UG_U8 objcnt;
   UG_OBJECT* objlst;
   UG_U8 state;
   UG_COLOR fc;
   UG_COLOR bc;
   UG_S16 xs;
   UG_S16 ys;
   UG_S16 xe;
   UG_S16 ye;
   UG_U8 style;
   UG_TITLE title;
   void (*cb)( UG_MESSAGE* );
};

/* Window states */
#define WND_STATE_FREE                                (1<<0)
#define WND_STATE_VALID                               (1<<1)
#define WND_STATE_BUSY                                (1<<2)
#define WND_STATE_VISIBLE                             (1<<3)
#define WND_STATE_ENABLE                              (1<<4)
#define WND_STATE_UPDATE                              (1<<5)
#define WND_STATE_REDRAW_TITLE                        (1<<6)

/* Window styles */
#define WND_STYLE_2D                                  (0<<0)
#define WND_STYLE_3D                                  (1<<0)
#define WND_STYLE_HIDE_TITLE                          (0<<1)
#define WND_STYLE_SHOW_TITLE                          (1<<1)

/* -------------------------------------------------------------------------------- */
/* -- BUTTON OBJECT                                                              -- */
/* -------------------------------------------------------------------------------- */
/* Button structure */
typedef struct
{
   UG_U8 state;
   UG_U8 style;
   UG_COLOR fc;
   UG_COLOR bc;
   UG_COLOR afc;
   UG_COLOR abc;
   const UG_FONT* font;
   char* str;
}UG_BUTTON;

/* Default button IDs */
#define BTN_ID_0                                      OBJ_ID_0
#define BTN_ID_1                                      OBJ_ID_1
#define BTN_ID_2                                      OBJ_ID_2
#define BTN_ID_3                                      OBJ_ID_3
#define BTN_ID_4                                      OBJ_ID_4
#define BTN_ID_5                                      OBJ_ID_5
#define BTN_ID_6                                      OBJ_ID_6
#define BTN_ID_7                                      OBJ_ID_7
#define BTN_ID_8                                      OBJ_ID_8
#define BTN_ID_9                                      OBJ_ID_9
#define BTN_ID_10                                     OBJ_ID_10
#define BTN_ID_11                                     OBJ_ID_11
#define BTN_ID_12                                     OBJ_ID_12
#define BTN_ID_13                                     OBJ_ID_13
#define BTN_ID_14                                     OBJ_ID_14
#define BTN_ID_15                                     OBJ_ID_15
#define BTN_ID_16                                     OBJ_ID_16
#define BTN_ID_17                                     OBJ_ID_17
#define BTN_ID_18                                     OBJ_ID_18
#define BTN_ID_19                                     OBJ_ID_19

/* Button states */
#define BTN_STATE_RELEASED                            (0<<0)
#define BTN_STATE_PRESSED                             (1<<0)
#define BTN_STATE_ALWAYS_REDRAW                       (1<<1)

/* Button style */
#define BTN_STYLE_2D                                  (0<<0)
#define BTN_STYLE_3D                                  (1<<0)
#define BTN_STYLE_TOGGLE_COLORS                       (1<<1)
#define BTN_STYLE_USE_ALTERNATE_COLORS                (1<<2)

/* Button events */
#define BTN_EVENT_CLICKED                             OBJ_EVENT_CLICKED

/* -------------------------------------------------------------------------------- */
/* -- TEXTBOX OBJECT                                                             -- */
/* -------------------------------------------------------------------------------- */
/* Textbox structure */
typedef struct
{
   char* str;
   const UG_FONT* font;
   UG_U8 style;
   UG_COLOR fc;
   UG_COLOR bc;
   UG_U8 align;
   UG_S8 h_space;
   UG_S8 v_space;
} UG_TEXTBOX;

/* Default textbox IDs */
#define TXB_ID_0                                      OBJ_ID_0
#define TXB_ID_1                                      OBJ_ID_1
#define TXB_ID_2                                      OBJ_ID_2
#define TXB_ID_3                                      OBJ_ID_3
#define TXB_ID_4                                      OBJ_ID_4
#define TXB_ID_5                                      OBJ_ID_5
#define TXB_ID_6                                      OBJ_ID_6
#define TXB_ID_7                                      OBJ_ID_7
#define TXB_ID_8                                      OBJ_ID_8
#define TXB_ID_9                                      OBJ_ID_9
#define TXB_ID_10                                     OBJ_ID_10
#define TXB_ID_11                                     OBJ_ID_11
#define TXB_ID_12                                     OBJ_ID_12
#define TXB_ID_13                                     OBJ_ID_13
#define TXB_ID_14                                     OBJ_ID_14
#define TXB_ID_15                                     OBJ_ID_15
#define TXB_ID_16                                     OBJ_ID_16
#define TXB_ID_17                                     OBJ_ID_17
#define TXB_ID_18                                     OBJ_ID_18
#define TXB_ID_19                                     OBJ_ID_19

/* -------------------------------------------------------------------------------- */
/* -- IMAGE OBJECT                                                               -- */
/* -------------------------------------------------------------------------------- */
/* Image structure */
typedef struct
{
   void* img;
   UG_U8 type;
} UG_IMAGE;

/* Default image IDs */
#define IMG_ID_0                                      OBJ_ID_0
#define IMG_ID_1                                      OBJ_ID_1
#define IMG_ID_2                                      OBJ_ID_2
#define IMG_ID_3                                      OBJ_ID_3
#define IMG_ID_4                                      OBJ_ID_4
#define IMG_ID_5                                      OBJ_ID_5
#define IMG_ID_6                                      OBJ_ID_6
#define IMG_ID_7                                      OBJ_ID_7
#define IMG_ID_8                                      OBJ_ID_8
#define IMG_ID_9                                      OBJ_ID_9
#define IMG_ID_10                                     OBJ_ID_10
#define IMG_ID_11                                     OBJ_ID_11
#define IMG_ID_12                                     OBJ_ID_12
#define IMG_ID_13                                     OBJ_ID_13
#define IMG_ID_14                                     OBJ_ID_14
#define IMG_ID_15                                     OBJ_ID_15
#define IMG_ID_16                                     OBJ_ID_16
#define IMG_ID_17                                     OBJ_ID_17
#define IMG_ID_18                                     OBJ_ID_18
#define IMG_ID_19                                     OBJ_ID_19

/* Image types */
#define IMG_TYPE_BMP                                  (1<<0)

/* -------------------------------------------------------------------------------- */
/* -- �GUI DRIVER                                                                -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
  void* driver;
  UG_U8 state;
} UG_DRIVER;

#define DRIVER_REGISTERED                             (1<<0)
#define DRIVER_ENABLED                                (1<<1)

/* Supported drivers */
#define NUMBER_OF_DRIVERS                             2
#define DRIVER_DRAW_LINE                              0
#define DRIVER_FILL_FRAME                             1

/* -------------------------------------------------------------------------------- */
/* -- �GUI CORE STRUCTURE                                                        -- */
/* -------------------------------------------------------------------------------- */
typedef struct
{
   void (*pset)(UG_S16,UG_S16,UG_COLOR);
   UG_S16 x_dim;
   UG_S16 y_dim;
   UG_TOUCH touch;
   UG_WINDOW* next_window;
   UG_WINDOW* active_window;
   UG_WINDOW* last_window;
   struct
   {
      UG_S16 x_pos;
      UG_S16 y_pos;
      UG_S16 x_start;
      UG_S16 y_start;
      UG_S16 x_end;
      UG_S16 y_end;
      UG_COLOR fore_color;
      UG_COLOR back_color;
   } console;
   struct
   {
      unsigned char* p;
      UG_S16 char_width;
      UG_S16 char_height;
      UG_S8 char_h_space;
      UG_S8 char_v_space;
   } font;
   UG_COLOR fore_color;
   UG_COLOR back_color;
   UG_COLOR desktop_color;
   UG_U8 state;
   UG_DRIVER driver[NUMBER_OF_DRIVERS];
} UG_GUI;

#define UG_SATUS_WAIT_FOR_UPDATE                      (1<<0)

/* -------------------------------------------------------------------------------- */
/* -- �GUI COLORS                                                                -- */
/* -- Source: http://www.rapidtables.com/web/color/RGB_Color.htm                 -- */
/* -------------------------------------------------------------------------------- */
#define  C_MAROON                     0x800000
#define  C_DARK_RED                   0x8B0000
#define  C_BROWN                      0xA52A2A
#define  C_FIREBRICK                  0xB22222
#define  C_CRIMSON                    0xDC143C
#define  C_RED                        0xFF0000
#define  C_TOMATO                     0xFF6347
#define  C_CORAL                      0xFF7F50
#define  C_INDIAN_RED                 0xCD5C5C
#define  C_LIGHT_CORAL                0xF08080
#define  C_DARK_SALMON                0xE9967A
#define  C_SALMON                     0xFA8072
#define  C_LIGHT_SALMON               0xFFA07A
#define  C_ORANGE_RED                 0xFF4500
#define  C_DARK_ORANGE                0xFF8C00
#define  C_ORANGE                     0xFFA500
#define  C_GOLD                       0xFFD700
#define  C_DARK_GOLDEN_ROD            0xB8860B
#define  C_GOLDEN_ROD                 0xDAA520
#define  C_PALE_GOLDEN_ROD            0xEEE8AA
#define  C_DARK_KHAKI                 0xBDB76B
#define  C_KHAKI                      0xF0E68C
#define  C_OLIVE                      0x808000
#define  C_YELLOW                     0xFFFF00
#define  C_YELLOW_GREEN               0x9ACD32
#define  C_DARK_OLIVE_GREEN           0x556B2F
#define  C_OLIVE_DRAB                 0x6B8E23
#define  C_LAWN_GREEN                 0x7CFC00
#define  C_CHART_REUSE                0x7FFF00
#define  C_GREEN_YELLOW               0xADFF2F
#define  C_DARK_GREEN                 0x006400
#define  C_GREEN                      0x00FF00
#define  C_FOREST_GREEN               0x228B22
#define  C_LIME                       0x00FF00
#define  C_LIME_GREEN                 0x32CD32
#define  C_LIGHT_GREEN                0x90EE90
#define  C_PALE_GREEN                 0x98FB98
#define  C_DARK_SEA_GREEN             0x8FBC8F
#define  C_MEDIUM_SPRING_GREEN        0x00FA9A
#define  C_SPRING_GREEN               0x00FF7F
#define  C_SEA_GREEN                  0x2E8B57
#define  C_MEDIUM_AQUA_MARINE         0x66CDAA
#define  C_MEDIUM_SEA_GREEN           0x3CB371
#define  C_LIGHT_SEA_GREEN            0x20B2AA
#define  C_DARK_SLATE_GRAY            0x2F4F4F
#define  C_TEAL                       0x008080
#define  C_DARK_CYAN                  0x008B8B
#define  C_AQUA                       0x00FFFF
#define  C_CYAN                       0x00FFFF
#define  C_LIGHT_CYAN                 0xE0FFFF
#define  C_DARK_TURQUOISE             0x00CED1
#define  C_TURQUOISE                  0x40E0D0
#define  C_MEDIUM_TURQUOISE           0x48D1CC
#define  C_PALE_TURQUOISE             0xAFEEEE
#define  C_AQUA_MARINE                0x7FFFD4
#define  C_POWDER_BLUE                0xB0E0E6
#define  C_CADET_BLUE                 0x5F9EA0
#define  C_STEEL_BLUE                 0x4682B4
#define  C_CORN_FLOWER_BLUE           0x6495ED
#define  C_DEEP_SKY_BLUE              0x00BFFF
#define  C_DODGER_BLUE                0x1E90FF
#define  C_LIGHT_BLUE                 0xADD8E6
#define  C_SKY_BLUE                   0x87CEEB
#define  C_LIGHT_SKY_BLUE             0x87CEFA
#define  C_MIDNIGHT_BLUE              0x191970
#define  C_NAVY                       0x000080
#define  C_DARK_BLUE                  0x00008B
#define  C_MEDIUM_BLUE                0x0000CD
#define  C_BLUE                       0x0000FF
#define  C_ROYAL_BLUE                 0x4169E1
#define  C_BLUE_VIOLET                0x8A2BE2
#define  C_INDIGO                     0x4B0082
#define  C_DARK_SLATE_BLUE            0x483D8B
#define  C_SLATE_BLUE                 0x6A5ACD
#define  C_MEDIUM_SLATE_BLUE          0x7B68EE
#define  C_MEDIUM_PURPLE              0x9370DB
#define  C_DARK_MAGENTA               0x8B008B
#define  C_DARK_VIOLET                0x9400D3
#define  C_DARK_ORCHID                0x9932CC
#define  C_MEDIUM_ORCHID              0xBA55D3
#define  C_PURPLE                     0x800080
#define  C_THISTLE                    0xD8BFD8
#define  C_PLUM                       0xDDA0DD
#define  C_VIOLET                     0xEE82EE
#define  C_MAGENTA                    0xFF00FF
#define  C_ORCHID                     0xDA70D6
#define  C_MEDIUM_VIOLET_RED          0xC71585
#define  C_PALE_VIOLET_RED            0xDB7093
#define  C_DEEP_PINK                  0xFF1493
#define  C_HOT_PINK                   0xFF69B4
#define  C_LIGHT_PINK                 0xFFB6C1
#define  C_PINK                       0xFFC0CB
#define  C_ANTIQUE_WHITE              0xFAEBD7
#define  C_BEIGE                      0xF5F5DC
#define  C_BISQUE                     0xFFE4C4
#define  C_BLANCHED_ALMOND            0xFFEBCD
#define  C_WHEAT                      0xF5DEB3
#define  C_CORN_SILK                  0xFFF8DC
#define  C_LEMON_CHIFFON              0xFFFACD
#define  C_LIGHT_GOLDEN_ROD_YELLOW    0xFAFAD2
#define  C_LIGHT_YELLOW               0xFFFFE0
#define  C_SADDLE_BROWN               0x8B4513
#define  C_SIENNA                     0xA0522D
#define  C_CHOCOLATE                  0xD2691E
#define  C_PERU                       0xCD853F
#define  C_SANDY_BROWN                0xF4A460
#define  C_BURLY_WOOD                 0xDEB887
#define  C_TAN                        0xD2B48C
#define  C_ROSY_BROWN                 0xBC8F8F
#define  C_MOCCASIN                   0xFFE4B5
#define  C_NAVAJO_WHITE               0xFFDEAD
#define  C_PEACH_PUFF                 0xFFDAB9
#define  C_MISTY_ROSE                 0xFFE4E1
#define  C_LAVENDER_BLUSH             0xFFF0F5
#define  C_LINEN                      0xFAF0E6
#define  C_OLD_LACE                   0xFDF5E6
#define  C_PAPAYA_WHIP                0xFFEFD5
#define  C_SEA_SHELL                  0xFFF5EE
#define  C_MINT_CREAM                 0xF5FFFA
#define  C_SLATE_GRAY                 0x708090
#define  C_LIGHT_SLATE_GRAY           0x778899
#define  C_LIGHT_STEEL_BLUE           0xB0C4DE
#define  C_LAVENDER                   0xE6E6FA
#define  C_FLORAL_WHITE               0xFFFAF0
#define  C_ALICE_BLUE                 0xF0F8FF
#define  C_GHOST_WHITE                0xF8F8FF
#define  C_HONEYDEW                   0xF0FFF0
#define  C_IVORY                      0xFFFFF0
#define  C_AZURE                      0xF0FFFF
#define  C_SNOW                       0xFFFAFA
#define  C_BLACK                      0x000000
#define  C_DIM_GRAY                   0x696969
#define  C_GRAY                       0x808080
#define  C_DARK_GRAY                  0xA9A9A9
#define  C_SILVER                     0xC0C0C0
#define  C_LIGHT_GRAY                 0xD3D3D3
#define  C_GAINSBORO                  0xDCDCDC
#define  C_WHITE_SMOKE                0xF5F5F5
#define  C_WHITE                      0xFFFFFF

/* -------------------------------------------------------------------------------- */
/* -- PROTOTYPES                                                                 -- */
/* -------------------------------------------------------------------------------- */
/* Classic functions */
UG_S16 ug_init( UG_GUI* g, void (*p)(UG_S16,UG_S16,UG_COLOR), UG_S16 x, UG_S16 y );
UG_S16 ug_select_gui( UG_GUI* g );
void ug_font_select( const UG_FONT* font );
void ug_fill_screen( UG_COLOR c );
void ug_fill_frame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
void ug_fill_round_frame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_S16 r, UG_COLOR c );
void ug_draw_mesh( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
void ug_draw_frame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
void ug_draw_round_frame( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_S16 r, UG_COLOR c );
void ug_draw_pixel( UG_S16 x0, UG_S16 y0, UG_COLOR c );
void ug_draw_circle( UG_S16 x0, UG_S16 y0, UG_S16 r, UG_COLOR c );
void ug_fill_circle( UG_S16 x0, UG_S16 y0, UG_S16 r, UG_COLOR c );
void ug_draw_arc( UG_S16 x0, UG_S16 y0, UG_S16 r, UG_U8 s, UG_COLOR c );
void ug_draw_line( UG_S16 x1, UG_S16 y1, UG_S16 x2, UG_S16 y2, UG_COLOR c );
void ug_put_string( UG_S16 x, UG_S16 y, char* str );
void ug_put_char( char chr, UG_S16 x, UG_S16 y, UG_COLOR fc, UG_COLOR bc );
void ug_console_put_string( char* str );
void ug_console_set_area( UG_S16 xs, UG_S16 ys, UG_S16 xe, UG_S16 ye );
void ug_console_set_forecolor( UG_COLOR c );
void ug_console_set_backcolor( UG_COLOR c );
void ug_set_forecolor( UG_COLOR c );
void ug_set_backcolor( UG_COLOR c );
UG_S16 ug_get_x_dim( void );
UG_S16 ug_get_y_dim( void );
void ug_font_set_h_space( UG_U16 s );
void ug_font_set_v_space( UG_U16 s );

/* Miscellaneous functions */
void ug_wait_for_update( void );
void ug_update( void );
void ug_draw_bmp( UG_S16 xp, UG_S16 yp, UG_BMP* bmp );
void ug_touch_update( UG_S16 xp, UG_S16 yp, UG_U8 state );

/* Driver functions */
void ug_driver_register( UG_U8 type, void* driver );
void ug_driver_enable( UG_U8 type );
void ug_driver_disable( UG_U8 type );

/* Window functions */
UG_RESULT ug_window_create( UG_WINDOW* wnd, UG_OBJECT* objlst, UG_U8 objcnt, void (*cb)( UG_MESSAGE* ) );
UG_RESULT ug_window_delete( UG_WINDOW* wnd );
UG_RESULT ug_window_show( UG_WINDOW* wnd );
UG_RESULT ug_window_hide( UG_WINDOW* wnd );
UG_RESULT ug_window_resize( UG_WINDOW* wnd, UG_S16 xs, UG_S16 ys, UG_S16 xe, UG_S16 ye );
UG_RESULT ug_window_alert( UG_WINDOW* wnd );
UG_RESULT ug_window_set_forecolor( UG_WINDOW* wnd, UG_COLOR fc );
UG_RESULT ug_window_set_backcolor( UG_WINDOW* wnd, UG_COLOR bc );
UG_RESULT ug_window_set_title_text_color( UG_WINDOW* wnd, UG_COLOR c );
UG_RESULT ug_window_set_title_color( UG_WINDOW* wnd, UG_COLOR c );
UG_RESULT ug_window_set_title_inactive_text_color( UG_WINDOW* wnd, UG_COLOR c );
UG_RESULT ug_window_set_title_inactive_color( UG_WINDOW* wnd, UG_COLOR c );
UG_RESULT ug_window_set_title_text( UG_WINDOW* wnd, char* str );
UG_RESULT ug_window_set_title_text_font( UG_WINDOW* wnd, const UG_FONT* font );
UG_RESULT ug_window_set_title_text_h_space( UG_WINDOW* wnd, UG_S8 hs );
UG_RESULT ug_window_set_title_text_v_space( UG_WINDOW* wnd, UG_S8 vs );
UG_RESULT ug_window_set_title_text_alignment( UG_WINDOW* wnd, UG_U8 align );
UG_RESULT ug_window_set_title_height( UG_WINDOW* wnd, UG_U8 height );
UG_RESULT ug_window_set_x_start( UG_WINDOW* wnd, UG_S16 xs );
UG_RESULT ug_window_set_y_start( UG_WINDOW* wnd, UG_S16 ys );
UG_RESULT ug_window_set_x_end( UG_WINDOW* wnd, UG_S16 xe );
UG_RESULT ug_window_set_y_end( UG_WINDOW* wnd, UG_S16 ye );
UG_RESULT ug_window_set_style( UG_WINDOW* wnd, UG_U8 style );
UG_COLOR ug_window_get_forecolor( UG_WINDOW* wnd );
UG_COLOR ug_window_get_backcolor( UG_WINDOW* wnd );
UG_COLOR ug_window_get_title_text_color( UG_WINDOW* wnd );
UG_COLOR ug_window_get_title_color( UG_WINDOW* wnd );
UG_COLOR ug_window_get_title_inactive_text_color( UG_WINDOW* wnd );
UG_COLOR ug_window_get_title_inactive_color( UG_WINDOW* wnd );
char* ug_window_get_title_text( UG_WINDOW* wnd );
UG_FONT* ug_window_get_title_text_font( UG_WINDOW* wnd );
UG_S8 ug_window_get_title__text_h_space( UG_WINDOW* wnd );
UG_S8 ug_window_get_title_text_v_space( UG_WINDOW* wnd );
UG_U8 ug_window_get_title_text_alignment( UG_WINDOW* wnd );
UG_U8 ug_window_get_title_height( UG_WINDOW* wnd );
UG_S16 ug_window_get_x_start( UG_WINDOW* wnd );
UG_S16 ug_window_get_y_start( UG_WINDOW* wnd );
UG_S16 ug_window_get_x_end( UG_WINDOW* wnd );
UG_S16 ug_window_get_y_end( UG_WINDOW* wnd );
UG_U8 ug_window_get_style( UG_WINDOW* wnd );
UG_RESULT ug_window_get_area( UG_WINDOW* wnd, UG_AREA* a );
UG_S16 ug_window_get_inner_width( UG_WINDOW* wnd );
UG_S16 ug_window_get_outer_width( UG_WINDOW* wnd );
UG_S16 ug_window_get_inner_height( UG_WINDOW* wnd );
UG_S16 ug_window_get_outer_height( UG_WINDOW* wnd );

/* Button functions */
UG_RESULT ug_button_create( UG_WINDOW* wnd, UG_BUTTON* btn, UG_U8 id, UG_S16 xs, UG_S16 ys, UG_S16 xe, UG_S16 ye );
UG_RESULT ug_button_delete( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_button_show( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_button_hide( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_button_set_forecolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR fc );
UG_RESULT ug_button_set_backcolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR bc );
UG_RESULT ug_button_set_alternate_forecolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR afc );
UG_RESULT ug_button_set_alternate_backcolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR abc );
UG_RESULT ug_button_set_text( UG_WINDOW* wnd, UG_U8 id, char* str );
UG_RESULT ug_button_set_font( UG_WINDOW* wnd, UG_U8 id, const UG_FONT* font );
UG_RESULT ug_button_set_style( UG_WINDOW* wnd, UG_U8 id, UG_U8 style );
UG_COLOR ug_button_get_forecolor( UG_WINDOW* wnd, UG_U8 id );
UG_COLOR ug_button_get_backcolor( UG_WINDOW* wnd, UG_U8 id );
UG_COLOR ug_button_get_alternate_forecolor( UG_WINDOW* wnd, UG_U8 id );
UG_COLOR ug_button_get_alternate_backcolor( UG_WINDOW* wnd, UG_U8 id );
char* ug_button_get_text( UG_WINDOW* wnd, UG_U8 id );
UG_FONT* ug_button_get_font( UG_WINDOW* wnd, UG_U8 id );
UG_U8 ug_button_get_style( UG_WINDOW* wnd, UG_U8 id );

/* Textbox functions */
UG_RESULT ug_textbox_create( UG_WINDOW* wnd, UG_TEXTBOX* txb, UG_U8 id, UG_S16 xs, UG_S16 ys, UG_S16 xe, UG_S16 ye );
UG_RESULT ug_Textbox_delete( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_textbox_show( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_textbox_hide( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_textbox_set_forecolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR fc );
UG_RESULT ug_textbox_set_backcolor( UG_WINDOW* wnd, UG_U8 id, UG_COLOR bc );
UG_RESULT ug_textbox_set_text( UG_WINDOW* wnd, UG_U8 id, char* str );
UG_RESULT ug_textbox_set_font( UG_WINDOW* wnd, UG_U8 id, const UG_FONT* font );
UG_RESULT ug_textbox_set_h_space( UG_WINDOW* wnd, UG_U8 id, UG_S8 hs );
UG_RESULT ug_textbox_set_v_space( UG_WINDOW* wnd, UG_U8 id, UG_S8 vs );
UG_RESULT ug_textbox_set_alignment( UG_WINDOW* wnd, UG_U8 id, UG_U8 align );
UG_COLOR ug_textbox_get_forecolor( UG_WINDOW* wnd, UG_U8 id );
UG_COLOR ug_textbox_get_backcolor( UG_WINDOW* wnd, UG_U8 id );
char* ug_textbox_get_text( UG_WINDOW* wnd, UG_U8 id );
UG_FONT* ug_textbox_get_font( UG_WINDOW* wnd, UG_U8 id );
UG_S8 ug_textbox_get_h_space( UG_WINDOW* wnd, UG_U8 id );
UG_S8 ug_textbox_get_v_space( UG_WINDOW* wnd, UG_U8 id );
UG_U8 ug_textbox_get_alignment( UG_WINDOW* wnd, UG_U8 id );

/* Image functions */
UG_RESULT ug_image_create( UG_WINDOW* wnd, UG_IMAGE* img, UG_U8 id, UG_S16 xs, UG_S16 ys, UG_S16 xe, UG_S16 ye );
UG_RESULT ug_image_delete( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_image_show( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_image_hide( UG_WINDOW* wnd, UG_U8 id );
UG_RESULT ug_image_set_bmp( UG_WINDOW* wnd, UG_U8 id, const UG_BMP* bmp );



#endif
