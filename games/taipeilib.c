/* The Ace of Penguins - tplib.c
   Copyright (C) 1998, 2001 DJ Delorie

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#include <stdio.h>
#include <string.h>
#include "imagelib.h"
#include "taipeilib.h"
#include "cards.h"

unsigned char grid[GRID_SX][GRID_SY][GRID_SZ];

char *filename;
int layer;

extern int table_width, table_height;

void
load(int complain)
{
  int x, y, z;
  FILE *f;
  if (!filename) return;
  f = fopen(filename, "r");
  if (!f)
  {
    if (complain)
      perror(filename);
    return;
  }

  memset(grid, 0, sizeof(grid));
  layer = 0;
  while (fscanf(f, "%d%*[^0-9-]", &x) > 0)
  {
    if (x == 127)
      break;
    if (x <= 0)
    {
      x = -x;
      fscanf(f, "%d%*[^0-9-]%d%*[^0-9-]", &y, &z);
      if (layer < z)
	layer = z;
    }
    /* Bounds check before writing to grid */
    if (x >= 0 && x < GRID_SX && y >= 0 && y < GRID_SY && z >= 0 && z < GRID_SZ)
      grid[x][y][z] = 1;
  }
    
  invalidate(0, 0, table_width, table_height);
}

void
invalidate_tile(int x, int y, int z)
{
  invalidate(gx2x(x, z), gy2y(y, z), TILE_SX * display_scale, TILE_SY * display_scale);
}

// Cached scaled sprite sheet for current display_scale
static image *cached_scaled_spritesheet = NULL;
static int cached_spritesheet_scale = 0;

void
prescale_spritesheet()
{
  // Free old cached sprite sheet if it exists
  if (cached_scaled_spritesheet) {
    // Note: We don't free the actual image data since it may be referenced elsewhere
    // Just clear our pointer to it
    cached_scaled_spritesheet = NULL;
  }

  // Load sprite sheet at native size
  image *img = get_image("taipei-tiles", 0, 0, 0);

  // If display_scale is 1, no need to scale - just use native
  if (display_scale == 1) {
    cached_scaled_spritesheet = img;
    cached_spritesheet_scale = 1;
  } else {
    // Scale the sprite sheet once for the current display_scale
    int new_width = (int)(img->width * display_scale);
    int new_height = (int)(img->height * display_scale);
    cached_scaled_spritesheet = scale_image(img, new_width, new_height);
    cached_spritesheet_scale = display_scale;
  }
}

static void
tile_synth2(image *rv)
{
  image *img;
  int x, y, which;
  int bx, by, bw, bh;
  int xo, yo, dx, dy;
  int tw, th, txo, tyo;
  image_list *list = rv->list;
  int width = rv->width;
  int height = rv->height;

  // Calculate scale factor based on requested tile size vs base size (39x39)
  double scale = (double)width / 39.0;

  // Use the pre-scaled cached sprite sheet if available and at correct scale
  if (cached_scaled_spritesheet && cached_spritesheet_scale == display_scale) {
    img = cached_scaled_spritesheet;
  } else {
    // Fallback: load and scale on demand (shouldn't normally happen)
    img = get_image("taipei-tiles", 0, 0, 0);
    if (scale != 1.0) {
      int new_width = (int)(img->width * scale);
      int new_height = (int)(img->height * scale);
      img = scale_image(img, new_width, new_height);
    }
  }

  // Calculate tile layout parameters scaled from original values
  // Original values were for 39x39 tiles from 234x337 sprite sheet
  bx = (int)(5 * scale); by = (int)(5 * scale);
  bw = (int)(39 * scale); bh = (int)(39 * scale);
  xo = (int)(50 * scale); yo = (int)(6 * scale);
  dx = (int)(36 * scale); dy = (int)(36 * scale);
  tw = (int)(34 * scale); th = (int)(34 * scale);
  txo = (int)(4 * scale); tyo = (int)(1 * scale);

  which = list->synth_flags;

  x = (which/10 - 2) * dx + xo;
  y = (which%10 - 1) * dy + yo;

  // When zoomed, apply 1-pixel compensation to mask coordinates to avoid
  // interpolation artifacts at tile edges
  int mask_offset = (display_scale > 1) ? 1 : 0;

  put_image (img, bx, by, bw, bh, rv, -bx, -by, 0);
  put_mask  (img, bx + mask_offset, by + mask_offset, bw, bh, rv, -bx, -by, 0);
  if (which)
    put_image (img, x, y, tw, th, rv, txo-x, tyo-y, 0);
}

static image *
tile_synth(image_list *list, int type, int width, int height)
{
  image *rv;
  int bw, bh;

  for (rv = list->subimage[type]; rv; rv=rv->next)
    if (rv->width == width && rv->height == height)
      return rv;

  // Tile dimensions are just what was requested
  bw = width;
  bh = height;

  rv = alloc_synth_image(list, bw, bh, type);
  rv->synth_func = tile_synth2;
  return rv;
}

image_list tile_images[] = {
  { "bl", 1, 1, {0,0,0}, 0, tile_synth, 0 },

  { "n1", 1, 1, {0,0,0}, 0, tile_synth, 21 },
  { "n2", 1, 1, {0,0,0}, 0, tile_synth, 22 },
  { "n3", 1, 1, {0,0,0}, 0, tile_synth, 23 },
  { "n4", 1, 1, {0,0,0}, 0, tile_synth, 24 },
  { "n5", 1, 1, {0,0,0}, 0, tile_synth, 25 },
  { "n6", 1, 1, {0,0,0}, 0, tile_synth, 26 },
  { "n7", 1, 1, {0,0,0}, 0, tile_synth, 27 },
  { "n8", 1, 1, {0,0,0}, 0, tile_synth, 28 },
  { "n9", 1, 1, {0,0,0}, 0, tile_synth, 29 },

  { "p1", 1, 1, {0,0,0}, 0, tile_synth, 31 },
  { "p2", 1, 1, {0,0,0}, 0, tile_synth, 32 },
  { "p3", 1, 1, {0,0,0}, 0, tile_synth, 33 },
  { "p4", 1, 1, {0,0,0}, 0, tile_synth, 34 },
  { "p5", 1, 1, {0,0,0}, 0, tile_synth, 35 },
  { "p6", 1, 1, {0,0,0}, 0, tile_synth, 36 },
  { "p7", 1, 1, {0,0,0}, 0, tile_synth, 37 },
  { "p8", 1, 1, {0,0,0}, 0, tile_synth, 38 },
  { "p9", 1, 1, {0,0,0}, 0, tile_synth, 39 },

  { "b1", 1, 1, {0,0,0}, 0, tile_synth, 41 },
  { "b2", 1, 1, {0,0,0}, 0, tile_synth, 42 },
  { "b3", 1, 1, {0,0,0}, 0, tile_synth, 43 },
  { "b4", 1, 1, {0,0,0}, 0, tile_synth, 44 },
  { "b5", 1, 1, {0,0,0}, 0, tile_synth, 45 },
  { "b6", 1, 1, {0,0,0}, 0, tile_synth, 46 },
  { "b7", 1, 1, {0,0,0}, 0, tile_synth, 47 },
  { "b8", 1, 1, {0,0,0}, 0, tile_synth, 48 },
  { "b9", 1, 1, {0,0,0}, 0, tile_synth, 49 },

  { "xj", 1, 1, {0,0,0}, 0, tile_synth, 51 },
  { "xa", 1, 1, {0,0,0}, 0, tile_synth, 52 },
  { "xx", 1, 1, {0,0,0}, 0, tile_synth, 53 },

  { "cr", 1, 1, {0,0,0}, 0, tile_synth, 56 },
  { "cg", 1, 1, {0,0,0}, 0, tile_synth, 57 },
  { "cb", 1, 1, {0,0,0}, 0, tile_synth, 58 },
  { "ck", 1, 1, {0,0,0}, 0, tile_synth, 59 },

  { "sh", 1, 1, {0,0,0}, 0, tile_synth, 61 },
  { "sd", 1, 1, {0,0,0}, 0, tile_synth, 62 },
  { "sc", 1, 1, {0,0,0}, 0, tile_synth, 63 },
  { "ss", 1, 1, {0,0,0}, 0, tile_synth, 64 },

  { "du", 1, 1, {0,0,0}, 0, tile_synth, 66 },
  { "dd", 1, 1, {0,0,0}, 0, tile_synth, 67 },
  { "dl", 1, 1, {0,0,0}, 0, tile_synth, 68 },
  { "dr", 1, 1, {0,0,0}, 0, tile_synth, 69 },
  { 0 }
};

