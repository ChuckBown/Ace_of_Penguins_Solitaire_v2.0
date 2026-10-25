/* The Ace of Penguins - minesweeper.c
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
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cards.h"
#include "qscore.h"
#include "xwin.h"

#define C_EMPTY	 0
#define C_BOMB	 9
#define C_RBOMB	10
#define C_FLAG	11
#define C_HUH	12
#define C_FILL	13
#define C_WRONG	14
#define C_UNKNOWN 15

#define TD_W	17
#define TD_H	26
#define TD_PX	175
#define TD_PW	31
#define TD_PH	26

#define PENGUIN_NORMAL	0
#define PENGUIN_SHOCK	1
#define PENGUIN_DEAD	2
#define PENGUIN_COOL	3

static int  num_bombs[4] = {  6, 20, 45, 200 };
static int      count[4] = {  8, 11, 16,  32 };
static int      sizes[4] = { 48, 36, 24,  12 };
static Picture *cells[4];

static char grid[32][32];
static char bomb[32][32];
static char neighbors[32][32];

/* 11*36+3+2 = 401 */
#define TW 401
#define TM 26
#define TH (401+TM)

static int level=0, x0, y0, sz, ct, sc;
static Picture *splash, *title, *xlogo;

static int untagged=0, penguin=0;

static int auto_play_mode = 0;

static int title_y=0;

// Large display mode: 32x32 grid with 24px cells (double window size)
static int large_display_mode = 0;

// Track if any moves have been made (to avoid incrementing starts count unnecessarily)
static int moves_made = 0;

extern int table_width, table_height;

static inline int
BOMB(int x, int y)
{
  if (x < 0 || y < 0 || x >= ct || y >= ct)
    return 0;
  return bomb[x][y];
}

static inline int
GRID(int x, int y)
{
  if (x < 0 || y < 0 || x >= ct || y >= ct)
    return C_UNKNOWN;
  return grid[x][y];
}

static void
start_again()
{
  int fill[1024]; /* 32x32 */
  int i, j, x, y;

  // In large display mode: use level 3 grid (32x32) with level 2 cell size (24px)
  if (large_display_mode) {
    sz = 24;  // Use 24px cells (level 2 size)
    ct = 32;  // Use 32x32 grid (level 3 count)
    sc = 1;   // level 3 spacing
  } else {
    sz = sizes[level];
    ct = count[level];
    sc = 4-level;
  }

  // Use actual table_width for centering (not TW constant)
  x0 = y0 = (table_width-ct*sz-sc)/2 + sc;
  y0 += TD_H;
  srand(time(NULL));

  for (i=0; i<1024; i++)
    fill[i] = i;
  for (i=ct*ct-1; i>=0; i--)
  {
    int t;
    j = rand() % (ct*ct);
    t = fill[i];
    fill[i] = fill[j];
    fill[j] = t;
  }
  for (x=0; x<ct; x++)
    for (y=0; y<ct; y++)
    {
      i = x*ct+y;
      // In large display mode, use level 3 bomb count
      int bomb_count = large_display_mode ? num_bombs[3] : num_bombs[level];
      if (fill[i] < bomb_count)
	bomb[x][y] = 1;
      else
	bomb[x][y] = 0;
      grid[x][y] = C_FILL;
      neighbors[x][y] = 0;
    }

  for (x=0; x<ct; x++)
    for (y=0; y<ct; y++)
      neighbors[x][y] =
	BOMB(x-1,y-1) + BOMB(x-1,y) + BOMB(x-1,y+1)
	+ BOMB(x,y-1) + BOMB(x,y+1)
	  + BOMB(x+1,y-1) + BOMB(x+1,y) + BOMB(x+1,y+1);

  untagged = large_display_mode ? num_bombs[3] : num_bombs[level];
  penguin = PENGUIN_NORMAL;
  title_y = (y0-sc-TD_PH+1)/2;
  moves_made = 0;  // Reset for new game
  invalidate(0, 0, table_width, table_height);
  score_increment_starts();
}

static void
init()
{
  splash = get_picture("minesweeper");
  title = get_picture("minesweeper-t");
  xlogo = get_picture("minesweeper-x");
  cells[0] = get_picture("minesweeper-c48");
  cells[1] = get_picture("minesweeper-c36");
  cells[2] = get_picture("minesweeper-c24");
  cells[3] = get_picture("minesweeper-c12");
  level = 3;
  srand(time(NULL));
  score_init(xwin_get_argv0());
  start_again();
}

static void
pc(int dx, int dy, int sx, int sy, int w, int h)
{
  // In large display mode, use level 2 cells (24px) instead of level 3 cells (12px)
  int cell_index = large_display_mode ? 2 : level;
  put_picture(cells[cell_index], dx-sx+x0, dy-sy+y0, sx, sy, w, h);
}

static void
set_grid(int x, int y, int c)
{
  int xx = x*sz;
  int yy = y*sz;
  grid[x][y] = c;
  pc(xx, yy, c*sz, 0, sz, sz);
}

static void
set_penguin(int p)
{
  int x, y;
  penguin = p;
  x = TD_PX+penguin*TD_PW;
  put_picture(title, table_width/2-TD_PW/2-x, title_y, x, 0, TD_PW, TD_PH);
}

static void
show_untagged()
{
  char tmp[25];
  int i, x;
  if (untagged < 0)
    strcpy(tmp, "000");
  else
    sprintf(tmp, "%03d", untagged);
  for (i=0; i<3; i++)
  {
    x = TD_W*(tmp[i]-'0');
    put_picture(title, i*TD_W-x+x0, title_y, x, 0, TD_W, TD_H);
  }
}

static void
flood_fill(int x, int y)
{
  if (x<0 || y<0 || x>=ct || y>=ct)
    return;
  if (bomb[x][y] || grid[x][y] == C_FLAG)
    return;
  if (grid[x][y] < 9)
    return;
  set_grid(x, y, neighbors[x][y]);
  if (neighbors[x][y] == 0)
  {
    flood_fill(x-1, y-1);
    flood_fill(x-1, y+1);
    flood_fill(x+1, y-1);
    flood_fill(x+1, y+1);
    flood_fill(x-1, y);
    flood_fill(x+1, y);
    flood_fill(x, y-1);
    flood_fill(x, y+1);
  }
}

static void
check_for_win()
{
  int x, y;
  if (untagged != 0)
    return;
  for (x=0; x<ct; x++)
    for (y=0; y<ct; y++)
      if (grid[x][y] > 8 && grid[x][y] != C_FLAG)
	return;
  set_penguin(PENGUIN_COOL);
  score_increment_wins();
}

static void
do_bomb(int x, int y)
{
  set_grid(x, y, C_RBOMB);
  set_penguin(PENGUIN_DEAD);
  for (x=0; x<ct; x++)
    for (y=0; y<ct; y++)
    {
      if (bomb[x][y] && grid[x][y] == C_FILL)
	set_grid(x, y, C_BOMB);
      if (!bomb[x][y] && grid[x][y] == C_FLAG)
	set_grid(x, y, C_WRONG);
    }
}

static int
autoclick(int x, int y)
{
  int dx, dy, nflags=0, nfills=0, rv=0;
  for (dx=-1; dx<=1; dx++)
    for (dy=-1; dy<=1; dy++)
    {
      int g = GRID(x+dx,y+dy);
      if (g == C_FLAG)
	nflags++;
      if (g == C_FILL || g == C_HUH)
	nfills++;
    }
  if (nflags == neighbors[x][y])
  {
    for (dx=-1; dx<=1; dx++)
      for (dy=-1; dy<=1; dy++)
      {
	int g = GRID(x+dx,y+dy);
	if (g == C_FILL || g == C_HUH)
	{
	  rv=1;
	  if (bomb[x+dx][y+dy])
	    do_bomb(x+dx, y+dy);
	  else if (neighbors[x+dx][y+dy] == 0)
	    flood_fill(x+dx, y+dy);
	  else
	    set_grid(x+dx, y+dy, neighbors[x+dx][y+dy]);
	}
      }
  }
  else if (nfills+nflags == neighbors[x][y])
  {
    for (dx=-1; dx<=1; dx++)
      for (dy=-1; dy<=1; dy++)
      {
	int g = GRID(x+dx,y+dy);
	if (g == C_FILL || g == C_HUH)
	{
	  rv=1;
	  set_grid(x+dx, y+dy, C_FLAG);
	  untagged--;
	  show_untagged();
	}
      }
  }
  return rv;
}

static void
autoplay()
{
  int x, y, c=1;
  set_penguin(PENGUIN_SHOCK);
  while (c && penguin == PENGUIN_SHOCK)
  {
    c = 0;
    for (x=0; x<ct; x++)
      for (y=0; y<ct; y++)
	if (grid[x][y] < 9)
	{
	  c += autoclick(x, y);
	}
  }
  if (penguin == PENGUIN_SHOCK)
    set_penguin(PENGUIN_NORMAL);
  check_for_win();
}

static void
resize_window(int new_width, int new_height)
{
  table_width = new_width;
  table_height = new_height;
  xwin_resize(new_width, new_height);
  xwin_fixed_size(new_width, new_height);
}

static void
key(int k, int x, int y)
{
  if (k == 3 || k == 27 || k == 'q')
    exit(0);

  set_centered_pic(0);

  if (k == KEY_F(1) || k == 'h')
  {
    help("minesweeper.html");
  }

  if (k == 'a' && penguin == PENGUIN_NORMAL)
    autoplay();
  if (k == 'A' && penguin == PENGUIN_NORMAL)
  {
    auto_play_mode = !auto_play_mode;
    autoplay();
  }

  if (k == KEY_F(2) || k == 'r')
    start_again();

  if (k >= '1' && k <= '4')
  {
    // If no moves made yet, undo the start increment that's about to happen
    if (!moves_made) {
      score_decrement_starts();
    }
    level = k - '1';
    large_display_mode = 0;  // Disable large display mode when selecting normal levels
    resize_window(TW, TH);
    start_again();
  }

  if (k == '5')
  {
    // If no moves made yet, undo the start increment that's about to happen
    if (!moves_made) {
      score_decrement_starts();
    }
    // Large display mode: 32x32 grid with 24px cells
    large_display_mode = 1;
    level = 3;  // Set to level 3 for proper game logic
    // Calculate new window size: 32 cells * 24px + spacing + title area
    int game_width = 32 * 24 + 1;  // 769
    int new_width = game_width;
    int new_height = game_width + TM;  // Add title margin

    // Check if it fits on screen
    if (xwin_check_size_fits(new_width, new_height)) {
      resize_window(new_width, new_height);
      start_again();
    } else {
      // Screen too small, stay in current mode
      large_display_mode = 0;
    }
  }

  if (k >= '6' && k <= '9')
  {
    penguin = k - '6';
    set_penguin(penguin);
  }
}

static void
click(int x, int y, int b)
{
  if (y < y0-sc && x > table_width/2-TD_PW/2 && x < table_width/2+TD_PW/2)
  {
    if (b > 1)
    {
      if (penguin == PENGUIN_NORMAL)
	autoplay();
    }
    else
      start_again();
  }

  if (penguin == PENGUIN_DEAD || penguin == PENGUIN_COOL || y < y0-sc)
    return;

  set_penguin(PENGUIN_SHOCK);
}

static void
drop(int x, int y, int b)
{
  if (penguin == PENGUIN_DEAD || penguin == PENGUIN_COOL)
    return;

  set_penguin(PENGUIN_NORMAL);

  if (y < y0 || x < x0)
    return;
  x = (x-x0)/sz;
  y = (y-y0)/sz;
  if (x>=ct || y>=ct)
    return;

  // Mark that moves have been made
  moves_made = 1;

  if (b > 1)
  {
    if (grid[x][y] < 9)
    {
      autoclick(x, y);
    }
    switch (grid[x][y])
    {
    case C_FILL:
      set_grid(x, y, C_FLAG);
      untagged--;
      show_untagged();
      if (auto_play_mode) autoplay();
      break;
    case C_FLAG:
      set_grid(x, y, C_HUH);
      untagged++;
      show_untagged();
      break;
    case C_HUH:
      set_grid(x, y, C_FILL);
      break;
    }

    check_for_win();
    return;
  }

  if (grid[x][y] != C_FILL
      && grid[x][y] != C_FLAG
      && grid[x][y] != C_HUH)
    return;

  if (grid[x][y] == C_FLAG)
    return;

  if (bomb[x][y])
  {
    do_bomb(x, y);
    return;
  }

  if (neighbors[x][y])
  {
    set_grid(x, y, neighbors[x][y]);
    if (auto_play_mode) autoplay();
    check_for_win();
    return;
  }

  flood_fill(x, y);
  if (auto_play_mode) autoplay();
  check_for_win();
}

static void
put(int x, int y)
{
  int xx = x*sz;
  int yy = y*sz;
  int cx = grid[x][y] * sz;
  pc(xx, yy, cx, 0, sz, sz);
}

static void
redraw()
{
  int x, y, i;
  pc(-sc, -sc, sz-sc, sz-sc, sc, sc);
  for (x=0; x<ct; x++)
  {
    pc(x*sz, -sc, 0, sz-sc, sz, sc);
    pc(-sc, x*sz, sz-sc, 0, sc, sz);
    for (y=0; y<ct; y++)
      put(x, y);
  }
  set_penguin(penguin);
  show_untagged();
  put_picture(xlogo, table_width-xlogo->w-x0, title_y, 0, 0, xlogo->w, xlogo->h);
}

static FunctionMapping fmap[] = {
  { "click", (void *)click },
  { "drop", (void *)drop },
  { "init", (void *)init },
  { "key", (void *)key },
  { "redraw", (void *)redraw },
  { 0, 0 }
};

int
main(int argc, char **argv)
{
  // Set table dimensions before init_ace() so init() can use them
  table_width = TW;
  table_height = TH;

  register_imagelib(appimglib_imagelib);
  init_ace(argc, argv, fmap);
  init_table(TW, TH);
  table_loop();
}
