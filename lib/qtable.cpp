/* The Ace of Penguins - qtable.cpp
   Qt5 implementation of table.c high-level graphics functions
   Maintains C API compatibility via extern "C" wrappers 
   Copyright (C) 2026 Chuck Bown

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <QFileDialog>
#include <QString>
#include <QDir>

extern "C" {
#include "imagelib.h"
#include "cards.h"
#include "xwin.h"
#include "funcs.h"
#include "table.h"

/* Default option descriptors - games can override app_options by defining
   non-static app_options and using a constructor function to set it before main() */
OptionDesc *app_options = 0;
OptionDesc *xwin_options = 0;
}

// Debug mode
#define DEBUG_QTABLE 0

// External declarations from qwin.cpp
class QPixmap;
extern QPixmap *qt_backbuffer;

// ============================================================================
// CARD SYNTHESIS FUNCTIONS (from table.c)
// ============================================================================

// Card spot positions
static const char *suit_spots[] = {
  "15",
  "101:",
  "10151:",
  "00200:2:",
  "0020150:2:",
  "002005250:2:",
  "00201205250:2:",
  "0020032307270:2:",
  "002003231507270:2:",
  "00201104240626190:2:"
};

static int spot_xx[] = { 0, 50, 100 };
static int spot_yy[] = { 0, 16, 25, 33, 36, 50, 63, 66, 75, 83, 100 };

// Forward declarations
static void card_synth2(image *rv);
static image *card_synth(image_list *list, int type, int width, int height);
static void empty_synth2(image *img);
static image *empty_synth(image_list *list, int type, int width, int height);
static void back_synth2(image *img);
static image *back_synth(image_list *list, int type, int width, int height);

// Card synthesis implementation
static void card_synth2(image *rv)
{
  image *face_img, *img;
  int face, suit, color;
  static char face_chars[] = "a234567890jqk";
  static char suit_chars[] = "cdsh";
  int subw, subh, face_w, w;
  int width = rv->width, height = rv->height;
  image_list *list = rv->list;

  fill_image (rv, 0, 0, width, height, 255, 255, 255);

  face = strchr(face_chars, list->name[0]) - face_chars;
  suit = strchr(suit_chars, list->name[1]) - suit_chars;
  color = suit & 1;

  face_w = width*2/11;
  face_img = get_image((char *)"a-k", face_w*2, face_w*13, 0);
  face_w = face_img->width / face_img->list->across;

  if (face < 10 && width > 3*face_w)
    {
      const char *spots = suit_spots[face];
      int sw, sh;
      if (face == 0)
	{
	  sw = width;
	  sh = height;
	}
      else
	{
	  sw = (width - 2*face_w) / 3;
	  sh = (height - 2*face_w) / 4;
	}
      if (face == 0 && suit == 2)
	img = get_image((char *)"penguin", sw, sh, GI_NOT_BIGGER);
      else
	img = get_image((char *)"suits", sw, sh*4, GI_NOT_BIGGER);
      if (img)
	{
	  int spw, sph, spoh, spow;
	  sw = img->width / img->list->across;
	  sh = img->height / img->list->down;
	  spow = face_w + 2;
	  spoh = face_w*3/4 + 2;
	  spw = width - 2*spow - sw;
	  sph = height - 2*spoh - sh;
	  while (*spots)
	    {
	      int sx = spot_xx[spots[0]-'0'] * spw / 100 + spow;
	      int sy = spot_yy[spots[1]-'0'] * sph / 100 + spoh;
	      put_subimage (img, 0, suit, rv, sx, sy,
			    spot_yy[spots[1]-'0'] > 51 ? PUT_ROTATED : 0);
	      spots += 2;
	    }
	}
    }

  if (face >= 10 &&  width > 3*face_w)
    {
      int wm = face_w+2;
      int hm = face_w*3/4+2;
      int w2 = width-2*wm;
      int h2 = height-2*hm;
      image *kqj, *simg;
      static char *portrait[] = {(char *)"jack", (char *)"queen", (char *)"king"};

      fill_image (rv, wm, hm, w2, 1, 0, 0, 0);
      fill_image (rv, wm, hm, 1, h2, 0, 0, 0);
      fill_image (rv, wm, height-hm, w2, 1, 0, 0, 0);
      fill_image (rv, width-wm, hm, 1, h2, 0, 0, 0);

      simg = get_image((char *)"suits", w2/3, w2*4/3, 0);

      kqj = get_image(portrait[face-10], w2, h2/2, GI_NOT_BIGGER);
      if (!kqj)
	    kqj = get_image(portrait[face-10], w2, h2, GI_NOT_BIGGER);

      if (simg)
	  {
	    put_subimage (simg, 0, suit, rv, wm+2, hm+2, 0);
	    put_subimage (simg, 0, suit, rv, width-wm-1-simg->width, height-hm-1-simg->height/4, PUT_ROTATED);
	  }

      if (kqj && kqj->height <= h2/2)
	  {
	    put_subimage (kqj, 0, 0, rv, width-wm-kqj->width, height/2-kqj->height, 0);
	    put_subimage (kqj, 0, 0, rv, wm+1, (height+1)/2, PUT_ROTATED);
	  }
      else if (kqj && kqj->height <= h2/2+3)
	  {
	    put_subimage (kqj, 0, 0, rv, width-wm-kqj->width, hm+1, 0);
	    put_subimage (kqj, 0, 0, rv, wm+1, height-hm-kqj->height, PUT_ROTATED);
	  }
      else if (kqj)
	  {
	    put_subimage (kqj, 0, 0, rv, (width+1-kqj->width)/2, (height+1-kqj->height)/2, 0);
	  }
    }

  fill_image (rv, 0, 0, width, 1, 0, 0, 0);
  fill_image (rv, 0, 0, 1, height, 0, 0, 0);
  fill_image (rv, 0, height-1, width, 1, 0, 0, 0);
  fill_image (rv, width-1, 0, 1, height, 0, 0, 0);

  put_subimage (face_img, color, face, rv, 1, 2, 0);
  subw = face_img->width / face_img->list->across;
  subh = face_img->height / face_img->list->down;
  if (width > subw*2+4)
    put_subimage (face_img, color, face, rv, width-1-subw, height-2-subh, PUT_ROTATED);

  img = get_image((char *)"suits", subw-2, (subw-2)*4, GI_NOT_BIGGER);
  put_subimage (img, 0, suit, rv, 1+subw/2-img->width/2, 4+subh, 0);
  if (width > subw*2+4)
    put_subimage (img, 0, suit, rv,
		  width-1-subw/2-img->width/2, height-4-subh-img->height/img->list->down,
		  PUT_ROTATED);
}

static image *card_synth(image_list *list, int type, int width, int height)
{
  image *rv;
  static int minw=0, minh=0;

  for (rv = list->subimage[type]; rv; rv=rv->next)
    if (rv->width == width && rv->height == height)
      return rv;

  if (minw == 0)
    {
      image *val, *suit;
      int face_w = width*2/11;
      val = get_image((char *)"a-k", face_w*2, face_w*13, 0);
      suit = get_image((char *)"suits", 9, 9*4, 0);
      minw = val->width / val->list->across + 2;
      minh = val->height / val->list->down + suit->height / suit->list->down + 6;
    }

  if (width < minw)
    width = minw;
  if (height < minh)
    height = minh;

  rv = alloc_synth_image(list, width, height, type);
  rv->synth_func = card_synth2;

  // Actually call the synthesis function to draw the card!
  if (rv->synth_func) {
    rv->synth_func(rv);
  }

  return rv;
}

static void empty_synth2(image *img)
{
  fill_image (img, 0, 0, img->width, img->height, 0, 102, 0);
  fill_image (img, 0, img->height-1, img->width, 1, 0, 0, 0);
  fill_image (img, img->width-1, 0, 1, img->height, 0, 0, 0);
  fill_image (img, 0, 0, img->width, 1, 0, 204, 0);
  fill_image (img, 0, 0, 1, img->height, 0, 204, 0);
}

static image *empty_synth(image_list *list, int type, int width, int height)
{
  image *rv, *img;

  for (rv = list->subimage[type]; rv; rv=rv->next)
    if (rv->width == width && rv->height == height)
      return rv;

  rv = alloc_synth_image(list, width, height, type);
  rv->synth_func = empty_synth2;

  // Actually call the synthesis function to draw the empty slot!
  if (rv->synth_func) {
    rv->synth_func(rv);
  }

  return rv;
}

static void back_synth2(image *img)
{
  image *tile = get_image ((char *)"back-tile", 1, 1, 0);
  int xo, yo, size;
  int x, y, l, r;
  int xs[6];
  int x01, x23, x24, x25;

  for (x=0; x<img->width; x += tile->width)
    for (y=0; y<img->height; y += tile->height)
      put_image (tile, 0, 0, tile->width, tile->height, img, x, y, 0);
  fill_image (img, 0, img->height-1, img->width, 1, 0, 0, 0);
  fill_image (img, img->width-1, 0, 1, img->height, 0, 0, 0);
  fill_image (img, 0, 0, img->width, 1, 0, 0, 0);
  fill_image (img, 0, 0, 1, img->height, 0, 0, 0);

  if (img->width < img->height)
    size = img->width * 2/3;
  else
    size = img->height * 2/3;
  xo = (img->width - size) / 2;
  yo = (img->height - size) / 2;

  x = (size-1) * 203 / 256;
  x25 = size - 1 - x;
  x23 = x25 * 105 / 256;
  x24 = x25 - x23;

  x = (size-1) * 200 / 256;
  x01 = size - 1 - x;

  for (y=0; y<size; y++)
    {
      xs[0] = y * 192/256;
      xs[1] = xs[0] + x01;
      xs[2] = (size-1-y) * 203/256;
      xs[3] = xs[2] + x23;
      xs[4] = xs[2] + x24;
      xs[5] = xs[2] + x25;

      l = (xs[0] < xs[2]) ? xs[0] : xs[2];
      r = (xs[1] < xs[3]) ? xs[1] : xs[3];
      fill_image (img, l+xo, y+yo, r-l+1, 1, 0, 0, 0);

      l = (xs[0] > xs[4]) ? xs[0] : xs[4];
      r = (xs[1] > xs[5]) ? xs[1] : xs[5];
      fill_image (img, l+xo, y+yo, r-l+1, 1, 0, 0, 0);
    }
}

static image *back_synth(image_list *list, int type, int width, int height)
{
  image *rv, *img;

  for (rv = list->subimage[type]; rv; rv=rv->next)
    if (rv->width == width && rv->height == height)
      return rv;

  rv = alloc_synth_image(list, width, height, type);
  rv->synth_func = back_synth2;

  // Actually call the synthesis function to draw the card back!
  if (rv->synth_func) {
    rv->synth_func(rv);
  }

  return rv;
}

// Card images list - all 52 cards plus special cards
image_list card_images[] = {
  { (char *)"ac", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"ad", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"as", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"ah", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"2c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"2d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"2s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"2h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"3c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"3d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"3s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"3h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"4c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"4d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"4s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"4h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"5c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"5d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"5s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"5h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"6c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"6d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"6s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"6h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"7c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"7d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"7s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"7h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"8c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"8d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"8s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"8h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"9c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"9d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"9s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"9h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"0c", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"0d", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"0s", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"0h", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"jc", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"jd", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"js", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"jh", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"qc", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"qd", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"qs", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"qh", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"kc", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"kd", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"ks", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"kh", 1, 1, {0,0,0}, 0, card_synth, 0 },
  { (char *)"empty", 1, 1, {0,0,0}, 0, empty_synth, 0 },
  { (char *)"back", 1, 1, {0,0,0}, 0, back_synth, 0 },
  { 0 }
};

// Globals from table.c
// These are defined in qwin.cpp with C linkage, we just declare them here
extern int table_width;
extern int table_height;
extern int display_width;
extern int display_height;

// These are defined here
int table_type = TABLE_COLOR;
int font_width = 6;
int font_height = 13;

// Globals used by stack.c for card dimensions
int get_picture_default_width = 73;
int get_picture_default_height = 97;

// Help system global
int help_is_showing = 0;

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

extern "C" void init_ace(int argc, char **argv, FunctionMapping *funcs)
{
#if DEBUG_QTABLE
    printf("init_ace(argc=%d, funcs=%p)\n", argc, funcs);
#endif

    // Register callbacks from game
    extern void (*init_cb)();
    extern void (*redraw_cb)();
    extern void (*click_cb)(int x, int y, int b);
    extern void (*drop_cb)(int x, int y, int b);
    extern void (*drag_cb)(int x, int y, int b);
    extern void (*key_cb)(int x, int y, int b);
    extern void (*resize_cb)(int w, int h);
    extern void (*double_click_cb)(int x, int y, int b);

    // Parse FunctionMapping array (name/function pairs)
    if (funcs) {
        for (int i = 0; funcs[i].name; i++) {
            const char *name = funcs[i].name;
            void *func = funcs[i].function;

#if DEBUG_QTABLE
            printf("  Registering callback: %s = %p\n", name, func);
#endif

            if (strcmp(name, "init") == 0) {
                init_cb = (void (*)())func;
            } else if (strcmp(name, "redraw") == 0) {
                redraw_cb = (void (*)())func;
            } else if (strcmp(name, "click") == 0) {
                click_cb = (void (*)(int, int, int))func;
            } else if (strcmp(name, "drop") == 0) {
                drop_cb = (void (*)(int, int, int))func;
            } else if (strcmp(name, "drag") == 0) {
                drag_cb = (void (*)(int, int, int))func;
            } else if (strcmp(name, "key") == 0) {
                key_cb = (void (*)(int, int, int))func;
            } else if (strcmp(name, "resize") == 0) {
                resize_cb = (void (*)(int, int))func;
            } else if (strcmp(name, "double_click") == 0) {
                double_click_cb = (void (*)(int, int, int))func;
            }
        }
    }

    // ========================================================================
    // COMMAND LINE OPTION PARSING
    // ========================================================================
    // Parse command line options before initializing Qt
    // app_options and xwin_options are defined at top of file as global variables
    // Games can override app_options by defining extern and setting via constructor

    // Built-in table options
    static OptionDesc ace_options[] = {
        { (char*)"-width", OPTION_INTEGER, &table_width },
        { (char*)"-height", OPTION_INTEGER, &table_height },
        { 0, 0, 0 }
    };

    // Build options array (game options, xwin options, ace options)
    OptionDesc *options[5];
    int opt_idx = 0;
    if (app_options)
        options[opt_idx++] = app_options;
    if (xwin_options)
        options[opt_idx++] = xwin_options;
    options[opt_idx++] = ace_options;
    options[opt_idx++] = nullptr;

    // Parse command line arguments
    int errors = 0;
    int a;
    for (a = 1; a < argc; a++) {
        if (argv[a][0] != '-')
            break;  // No more options

        bool found = false;
        for (int i = 0; options[i]; i++) {
            for (int o = 0; options[i][o].option; o++) {
                if (strcmp(options[i][o].option, argv[a]) == 0) {
                    found = true;

                    // Check if option requires an argument
                    if (options[i][o].type != OPTION_BOOLEAN && a == argc - 1) {
                        fprintf(stderr, "Option `%s' takes an argument\n", argv[a]);
                        errors++;
                        continue;
                    }

                    // Process option based on type
                    switch (options[i][o].type) {
                        case OPTION_BOOLEAN:
                            *(int *)(options[i][o].ptr) = 1;
                            break;
                        case OPTION_STRING:
                            *(char **)(options[i][o].ptr) = argv[a + 1];
                            a++;
                            break;
                        case OPTION_INTEGER:
                            *(int *)(options[i][o].ptr) = (int)strtol(argv[a + 1], nullptr, 0);
                            a++;
                            break;
                    }
                    break;  // Found option, stop searching
                }
            }
            if (found) break;
        }

        if (!found) {
            fprintf(stderr, "Unrecognized option `%s'\n", argv[a]);
            errors++;
        }
    }

    if (errors) {
        exit(errors);
    }

    // Compact argv to remove processed options
    int new_argc = 1;
    while (a < argc) {
        argv[new_argc++] = argv[a++];
    }
    argv[new_argc] = nullptr;

    // Update argc for remaining arguments
    argc = new_argc;

    // IMPORTANT: Initialize Qt (create QApplication) FIRST
    // This must happen before loading any images (which create QPixmaps)
    xwin_init(argc, argv);

    // Register card synthesis library
    register_imagelib(card_images);
#if DEBUG_QTABLE
    printf("  Registered card_images[] with synthesis functions\n");
#endif

    // Call init callback if present
    // This may load images, so QApplication must already exist
    if (init_cb) {
#if DEBUG_QTABLE
        printf("  Calling init_cb()\n");
#endif
        init_cb();
    }
}

extern "C" void init_table(int width, int height)
{
#if DEBUG_QTABLE
    printf("STUB: init_table(width=%d, height=%d)\n", width, height);
#endif

    // TODO: Clamp to display dimensions
    // TODO: Create window

    if (width > display_width)
        width = display_width;
    if (height > display_height)
        height = display_height;

    table_width = width;
    table_height = height;

    xwin_create(width, height);
}

extern "C" void table_loop()
{
#if DEBUG_QTABLE
    printf("table_loop() - entering event loop\n");
#endif

    // Callback functions from funcs.h
    extern void (*redraw_cb)();
    extern void (*resize_cb)(int w, int h);
    extern void (*click_cb)(int x, int y, int b);
    extern void (*drop_cb)(int x, int y, int b);
    extern void (*drag_cb)(int x, int y, int b);
    extern void (*key_cb)(int x, int y, int b);

    XWin_Event event;
    Picture *cpic;  // Declare outside switch for centered picture handling
    image *img;

    while (1) {
        int ev_type = xwin_nextevent(&event);

        if (ev_type == ev_quit) {
#if DEBUG_QTABLE
            printf("Received quit event, exiting\n");
#endif
            exit(0);
        }

        // Dispatch events to game callbacks
        switch (event.type) {
            case ev_expose:
#if DEBUG_QTABLE
                printf("Got expose event - calling redraw_cb\n");
#endif
                // Clear the entire table background before full redraw
                clear(0, 0, table_width, table_height);

                if (redraw_cb) {
                    redraw_cb();
                }

                // Redraw centered picture (splash screen) on top if present
                cpic = get_centered_pic();
                if (cpic) {
                    img = (image *)cpic;
                    int x = (table_width - img->width) / 2;
                    int y = (table_height - img->height) / 2;
                    put_picture(cpic, x, y, 0, 0, img->width, img->height);
                }
                break;

            case ev_resize:
#if DEBUG_QTABLE
                printf("Got resize event %dx%d - calling resize_cb\n", event.w, event.h);
#endif
                // Clear the entire table background before resize redraw
                clear(0, 0, event.w, event.h);

                if (resize_cb) {
                    resize_cb(event.w, event.h);
                }

                // After resize callback, we need to redraw everything at the new size
                if (redraw_cb) {
#if DEBUG_QTABLE
                    printf("  Calling redraw_cb after resize\n");
#endif
                    redraw_cb();
                }

                // CRITICAL: Invalidate the entire window after resize to ensure
                // any areas not redrawn by game callbacks get properly cleared
                // This will also redraw the centered picture on top if present
                invalidate(0, 0, event.w, event.h);

                // Force immediate synchronous repaint after resize to show all the new cards
                flushsync();
                break;

            case ev_buttondown:
#if DEBUG_QTABLE
                printf("Got button down at (%d,%d) button %d\n", event.x, event.y, event.button);
#endif
                if (click_cb) {
                    click_cb(event.x, event.y, event.button);
                }
                break;

            case ev_buttonup:
#if DEBUG_QTABLE
                printf("Got button up at (%d,%d) button %d\n", event.x, event.y, event.button);
#endif
                if (drop_cb) {
                    drop_cb(event.x, event.y, event.button);
                }
                break;

            case ev_motion:
                // Only call drag if button is pressed (check shifts field)
                if (event.shifts & (0x0100 | 0x0200 | 0x0400)) {
                    if (drag_cb) {
                        drag_cb(event.x, event.y, event.button);
                    }
                }
                break;

            case ev_keypress:
#if DEBUG_QTABLE
                printf("Got keypress '%c' (0x%02x), key_cb=%p\n", event.key, event.key, key_cb);
#endif
                if (key_cb) {
#if DEBUG_QTABLE
                    printf("  Calling key_cb(%d, %d, %d) - KEY FIRST!\n", event.key, event.x, event.y);
#endif
                    // IMPORTANT: Game expects key(k, x, y) not key(x, y, k)!
                    key_cb(event.key, event.x, event.y);
#if DEBUG_QTABLE
                    printf("  key_cb returned\n");
#endif
                }
                break;

            case ev_doubleclick:
#if DEBUG_QTABLE
                printf("Got double-click at (%d,%d) button %d\n", event.x, event.y, event.button);
#endif
                if (double_click_cb) {
                    double_click_cb(event.x, event.y, event.button);
                }
                break;

            default:
                break;
        }
    }
}

extern "C" void table_no_resize()
{
#if DEBUG_QTABLE
    printf("STUB: table_no_resize()\n");
#endif

    // TODO: Set flag to prevent resizing
    // TODO: Call xwin_fixed_size()

    xwin_fixed_size(table_width, table_height);
}

// ============================================================================
// PICTURE FUNCTIONS
// ============================================================================

extern "C" Picture *get_picture(char *name)
{
#if DEBUG_QTABLE
    printf("get_picture(name='%s')\n", name);
#endif

    // Picture* is actually image* - they're the same type
    // Call get_image() to load the PNG file
    Picture *result = (Picture *)get_image(name, get_picture_default_width, get_picture_default_height, 0);

#if DEBUG_QTABLE
    if (result) {
        printf("  get_picture: loaded successfully (%p)\n", result);
    } else {
        printf("  get_picture: FAILED to load '%s'\n", name);
    }
#endif

    return result;
}

extern "C" void put_picture(Picture *picture, int dx, int dy,
                            int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    static int call_count = 0;
    if (call_count++ < 5) {
        printf("put_picture(picture=%p, dx=%d, dy=%d, x=%d, y=%d, w=%d, h=%d)\n",
               picture, dx, dy, x, y, w, h);
    }
#endif

    if (!picture) {
        return;
    }

    // Picture* is actually image* - call put_image to render it
    // display_image is NULL means render to the backbuffer (which put_image does by default)
    extern image *display_image;
    put_image((image *)picture, x, y, w, h, display_image, dx, dy, 0);
}

extern "C" void put_picture_inverted(Picture *picture, int dx, int dy,
                                     int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    printf("put_picture_inverted(picture=%p, dx=%d, dy=%d, x=%d, y=%d, w=%d, h=%d)\n",
           picture, dx, dy, x, y, w, h);
#endif

    if (!picture) {
        return;
    }

    // Picture* is actually image* - call put_image with PUT_INVERTED flag
    extern image *display_image;
    put_image((image *)picture, x, y, w, h, display_image, dx, dy, PUT_INVERTED);
}

// Static storage for centered picture (splash screen)
static Picture *centered_picture = nullptr;

// Non-static so help system can access it
Picture *last_cleared_picture = nullptr;  // Save for help system restore

extern "C" void set_centered_pic(Picture *picture)
{
#if DEBUG_QTABLE
    printf("set_centered_pic(picture=%p)\n", picture);
#endif

    // Store the centered picture (splash screen)
    Picture *old_picture = centered_picture;
    centered_picture = picture;

    // Save last cleared picture for help system restore
    // ONLY save if we're actually clearing a picture that was showing
    // (Don't update if old_picture was already NULL - keeps stale picture from being restored)
    if (old_picture && !picture) {
        last_cleared_picture = old_picture;
    } else if (picture) {
        // New picture set, clear the saved one
        last_cleared_picture = nullptr;
    } else if (!old_picture && !picture) {
        // Both NULL - clear stale saved picture
        last_cleared_picture = nullptr;
    }

    // If we're setting a new picture, draw it centered
    // BUT only if backbuffer is initialized - otherwise paintEvent will draw it
    if (picture && qt_backbuffer) {
        image *img = (image *)picture;
        int x = (table_width - img->width) / 2;
        int y = (table_height - img->height) / 2;

#if DEBUG_QTABLE
        printf("  Drawing centered picture at (%d, %d) size %dx%d\n", x, y, img->width, img->height);
#endif

        // Draw the picture centered on the table
        put_picture(picture, x, y, 0, 0, img->width, img->height);
    }
#if DEBUG_QTABLE
    else if (picture && !qt_backbuffer) {
        printf("  Backbuffer not ready yet - picture will be drawn on first paintEvent\n");
    }
#endif

    // If we're removing the old picture, invalidate that region to redraw
    if (old_picture && !picture) {
        image *old_img = (image *)old_picture;
        int old_x = (table_width - old_img->width) / 2;
        int old_y = (table_height - old_img->height) / 2;

#if DEBUG_QTABLE
        printf("  Clearing splash: invalidating region (%d,%d,%d,%d)\n",
               old_x, old_y, old_img->width, old_img->height);
#endif

        // Invalidate the old region to trigger redraw
        invalidate(old_x, old_y, old_img->width, old_img->height);

        // Also trigger a flush to ensure the update is visible
        flushsync();
    }
}

extern "C" Picture *get_centered_pic()
{
#if DEBUG_QTABLE
    printf("get_centered_pic() -> %p\n", centered_picture);
#endif

    return centered_picture;
}

// ============================================================================
// CLIPPING AND INVALIDATION
// ============================================================================

extern "C" void clip(int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    printf("clip(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Set clipping region
    // Clamp to table boundaries
    extern int table_width;
    extern int table_height;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > table_width) w = table_width - x;
    if (y + h > table_height) h = table_height - y;

    if (w > 0 && h > 0) {
        xwin_clip(x, y, w, h);
    }
}

extern "C" void clip_more(int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    printf("clip_more(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Intersect current clip region with new region
    // For now, just set the new region
    // TODO: Implement proper clip region stack if needed
    clip(x, y, w, h);
}

extern "C" void unclip()
{
#if DEBUG_QTABLE
    printf("unclip()\n");
#endif

    // Restore previous clip region (disable clipping)
    xwin_noclip();
}

extern "C" void invalidate(int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    printf("invalidate(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Trigger redraw of specified region
    extern void (*redraw_cb)();

    // Set clipping to the invalidated region
    clip(x, y, w, h);

    // CRITICAL: Clear the background first to remove old graphics
    clear(x, y, w, h);

    // Call the game's redraw callback to redraw this region
    if (redraw_cb) {
        redraw_cb();
    }

    // Redraw centered picture (splash screen, youwin, etc.) on top if present
    Picture *cpic = get_centered_pic();
    if (cpic) {
        image *img = (image *)cpic;
        int cx = (table_width - img->width) / 2;
        int cy = (table_height - img->height) / 2;

#if DEBUG_QTABLE
        printf("DEBUG: invalidate() found centered pic at (%d,%d) size %dx%d, invalidated region (%d,%d,%d,%d)\n",
               cx, cy, img->width, img->height, x, y, w, h);
#endif

        // Check if centered picture overlaps with invalidated region
        if (cx < x + w && cx + img->width > x &&
            cy < y + h && cy + img->height > y) {
#if DEBUG_QTABLE
            printf("DEBUG: invalidate() redrawing centered picture\n");
#endif
            put_picture(cpic, cx, cy, 0, 0, img->width, img->height);
        }
#if DEBUG_QTABLE
        else {
            printf("DEBUG: invalidate() centered picture does NOT overlap with invalidated region\n");
        }
#endif
    }
#if DEBUG_QTABLE
    else {
        printf("DEBUG: invalidate() no centered pic found\n");
    }
#endif

    // Restore clipping
    unclip();

    // Trigger screen update to display the redrawn region
    flush();
}

extern "C" void invalidate_nc(int x, int y, int w, int h)
{
#if DEBUG_QTABLE
    printf("invalidate_nc(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Invalidate without clearing - just trigger redraw without clip
    // This is used when the area doesn't need to be cleared first
    extern void (*redraw_cb)();

    // Call the game's redraw callback without setting clip region
    // The game will redraw the specified area
    if (redraw_cb) {
        redraw_cb();
    }

    // Trigger screen update to display the redrawn region
    flush();
}

extern "C" void invalidate_exposure(int ox, int oy, int ow, int oh,
                                    int nx, int ny, int nw, int nh)
{
#if DEBUG_QTABLE
    printf("invalidate_exposure(ox=%d, oy=%d, ow=%d, oh=%d, nx=%d, ny=%d, nw=%d, nh=%d)\n",
           ox, oy, ow, oh, nx, ny, nw, nh);
#endif

    // Invalidate region exposed by moving object
    // This happens when a card moves from old position (ox,oy,ow,oh) to new position (nx,ny,nw,nh)

    extern void (*redraw_cb)();

    // Calculate the union of old and new rectangles
    int min_x = (ox < nx) ? ox : nx;
    int min_y = (oy < ny) ? oy : ny;
    int max_x = ((ox + ow) > (nx + nw)) ? (ox + ow) : (nx + nw);
    int max_y = ((oy + oh) > (ny + nh)) ? (oy + oh) : (ny + nh);

    int union_w = max_x - min_x;
    int union_h = max_y - min_y;

    // Set clipping to the union region
    clip(min_x, min_y, union_w, union_h);

    // CRITICAL: Clear the background first to remove old card graphics
    clear(min_x, min_y, union_w, union_h);

    // Call the game's redraw callback to redraw this region
    if (redraw_cb) {
        redraw_cb();
    }

    // Redraw centered picture (splash screen, youwin, etc.) on top if present
    Picture *cpic = get_centered_pic();
    if (cpic) {
        image *img = (image *)cpic;
        int cx = (table_width - img->width) / 2;
        int cy = (table_height - img->height) / 2;

        // Check if centered picture overlaps with invalidated region
        if (cx < min_x + union_w && cx + img->width > min_x &&
            cy < min_y + union_h && cy + img->height > min_y) {
            put_picture(cpic, cx, cy, 0, 0, img->width, img->height);
        }
    }

    // Restore clipping
    unclip();

    // Trigger screen update to display the redrawn region
    flush();
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

extern "C" void snap_to_grid(int *x, int *y,
                             int step_x, int step_y,
                             int origin_x, int origin_y,
                             int max_distance)
{
#if DEBUG_QTABLE
    printf("STUB: snap_to_grid()\n");
#endif

    // TODO: Snap coordinates to grid
    // This function is platform-independent, could keep original implementation
}

// help() is now implemented in qhelp.cpp
// Forward declaration - actual implementation in qhelp.cpp
extern "C" void help(char *filename);

// ============================================================================
// FILE DIALOG
// ============================================================================

extern "C" char *show_file_open_dialog(const char *title, const char *filter)
{
    // Determine starting directory
    // Windows: Current directory (executable location)
    // Linux: User's home directory
#ifdef _WIN32
    QString start_dir = QDir::currentPath();
#else
    QString start_dir = QDir::homePath();
#endif

#if DEBUG_QTABLE
    printf("show_file_open_dialog(title='%s', filter='%s', dir='%s')\n", title, filter,
           start_dir.toUtf8().constData());
#endif

    // Get parent window from qwin.cpp
    extern QWidget *qt_window;

    // Create dialog explicitly to set font size
    QFileDialog dialog(qt_window, title, start_dir, filter);
    dialog.setFileMode(QFileDialog::ExistingFile);
    dialog.setOptions(QFileDialog::DontUseNativeDialog);

    // Set font size
    QFont font = dialog.font();
    font.setPointSize(12);
    dialog.setFont(font);

    QString fname;
    if (dialog.exec()) {
        QStringList files = dialog.selectedFiles();
        if (!files.isEmpty())
            fname = files.first();
    }

    if (fname.isEmpty()) {
#if DEBUG_QTABLE
        printf("  User cancelled\n");
#endif
        return nullptr;
    }

#if DEBUG_QTABLE
    printf("  Selected file: %s\n", fname.toUtf8().constData());
#endif

    // Allocate C string (caller must free)
    QByteArray utf8 = fname.toUtf8();
    char *result = (char*)malloc(utf8.length() + 1);
    strcpy(result, utf8.constData());
    return result;
}
