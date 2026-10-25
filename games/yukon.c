/* The Ace of Penguins - yukon.c
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "imagelib.h"
#include "cards.h"
#include "qscore.h"

// Debug mode - prints auto-move logic details
#define DEBUG_YUKON 0

#define W CARD_WIDTH
#define H CARD_HEIGHT
#define M CARD_MARGIN
#define F CARD_FAN_DOWN

#define WIN_H 600
#define WIN_W 7*W + 8*M

extern int stack_fan_down;

static Picture *xlogo, *splash, *youwin, *youlose, *arrow, *no_arrow;
static Stack *deck, *outcells[4], *maincells[7];
static int hint_mode = 0;
static char values[] = " A23456789TJQK";
static char suits[] = "HDCS";

static int auto_move();
static void check_for_end_of_game();
static void set_arrows();

static int ax, adx, ay, ady;

static int xlogo_x;
static int xlogo_y;

static int supress_arrows = 0;
static int arrowsx[7] = { 0, 0, 0, 0, 0, 0, 0 };
static int arrowsy[7] = { 0, 0, 0, 0, 0, 0, 0 };

/* Foundation arrows - under the 4 outcells */
static int found_arrowsx[4] = { 0, 0, 0, 0 };
static int found_arrowsy[4] = { 0, 0, 0, 0 };  /* 0 = off, non-zero = on and at this y position */
static int found_arrow_ypos[4] = { 0, 0, 0, 0 };  /* Permanent y positions set in init/resize */

static void
resize(int w, int h)
{
  int margin, offset, cw, ch, s, fd, fr, tfd, tfr;
  Picture *empty;

  stack_set_card_size (w/8, w/8*4/3);
  stack_get_card_size (&cw, &ch);

  empty = (Picture *)get_image("empty", cw, ch, 0);

  margin = (w - 7*cw) / 8;
  offset = (w - margin*8 - cw*7) / 2 + margin;

  for (s=0; s<4; s++)
    {
      stack_move(outcells[s], w - (4-s)*(cw+margin), margin);
      stack_set_empty_picture(outcells[s], empty);
    }

  /* Move main stacks down to make room for foundation arrows */
  int arrow_space = arrow->h + 5;  /* arrow height plus small gap */
  for (s=0; s<7; s++)
    stack_move(maincells[s], offset + s*(cw+margin), ch + (offset<0?0:offset) + margin + arrow_space);

  stack_get_fans(&fd, &fr, &tfd, &tfr);

  if (xlogo->w > cw)
    xlogo_x = xlogo_y = 0;
  else
    {
      xlogo_x = 3 * margin + 2 * cw + cw/2 - xlogo->w/2;
      xlogo_y = margin + ch/2 - xlogo->h/2;
    }

  /* Position foundation arrows under each outcell */
  for (s=0; s<4; s++)
    {
      found_arrowsx[s] = w - (4-s)*(cw+margin) + cw/2;
      found_arrow_ypos[s] = margin + ch + 2;
    }

  /* Position main stack arrows (adjusted for new layout) */
  ax = offset + cw/2;
  adx = margin + cw;
  ay = (offset<0?0:offset) + ch*2 + margin + arrow_space + 2;
  ady = fd;

  set_arrows();
}

static void
set_arrow(int column, int on)
{
  int x = ax + adx*column;
  if (supress_arrows)
    return;
  if (arrowsy[column])
    put_picture(no_arrow, arrowsx[column]-no_arrow->w/2, arrowsy[column],
		0, 0, no_arrow->w, no_arrow->h);
  if (on)
  {
    arrowsx[column] = x;
    arrowsy[column] = ay+ady*(stack_count_cards(maincells[column])-1);
    put_picture(arrow, arrowsx[column]-arrow->w/2, arrowsy[column],
		0, 0, arrow->w, arrow->h);
  }
  else
    arrowsy[column] = 0;
}

static void
redraw_arrows()
{
  int i;
  for (i=0; i<7; i++)
  {
    if (arrowsy[i])
      put_picture(arrow, arrowsx[i]-arrow->w/2, arrowsy[i],
		  0, 0, arrow->w, arrow->h);
  }
}

static void
clear_arrows()
{
  int i;
  for (i=0; i<7; i++)
    set_arrow(i, 0);
}

static void
set_foundation_arrow(int column, int on)
{
  if (supress_arrows)
    return;
  /* Clear any existing arrow at this position */
  if (found_arrowsy[column])
    put_picture(no_arrow, found_arrowsx[column]-no_arrow->w/2, found_arrowsy[column],
		0, 0, no_arrow->w, no_arrow->h);
  if (on)
  {
    /* Turn arrow on - set y position from permanent position */
    found_arrowsy[column] = found_arrow_ypos[column];
    put_picture(arrow, found_arrowsx[column]-arrow->w/2, found_arrowsy[column],
		0, 0, arrow->w, arrow->h);
  }
  else
  {
    /* Turn arrow off - clear y position */
    found_arrowsy[column] = 0;
  }
}

static void
redraw_foundation_arrows()
{
  int i;
  if (supress_arrows)
    return;
  for (i=0; i<4; i++)
  {
    if (found_arrowsy[i])
      put_picture(arrow, found_arrowsx[i]-arrow->w/2, found_arrowsy[i],
		  0, 0, arrow->w, arrow->h);
  }
}

static void
clear_foundation_arrows()
{
  int i;
  for (i=0; i<4; i++)
    set_foundation_arrow(i, 0);
}

static int
can_move_to_foundation(int card)
{
  int f;

  /* Aces can always start a foundation */
  if (VALUE(card) == ACE)
    return 1;

  /* Check if matches any foundation top */
  for (f = 0; f < 4; f++)
    {
      int fc = stack_count_cards(outcells[f]);
      if (fc > 0)
	{
	  int ftop = stack_get_card(outcells[f], fc-1);
	  if (SUIT(card) == SUIT(ftop) && VALUE(card) == VALUE(ftop) + 1)
	    return 1;
	}
    }

  return 0;
}

static void
set_arrows()
{
  int i, j, k;
  int main_arrow_count = 0;

  /* First, process main stack arrows */
  for (i=0; i<7; i++)
  {
    Stack *s = maincells[i];
    int count = stack_count_cards(s);
    set_arrow(i, 0);

    if (count == 0)
    {
      /* Empty stack - look for kings that aren't already at bottom */
      int found = 0;
      for (j=0; j<7 && !found; j++)
	if (j != i)
	  for (k=stack_count_cards(maincells[j])-1; k>=0 && !found; k--)
	  {
	    int match = stack_get_card(maincells[j], k);
	    if (!FACEDOWNP(match) && VALUE(match) == KING && k > 0)
	    {
	      set_arrow(i, 1);
	      main_arrow_count++;
	      found = 1;
	    }
	  }
    }
    else
    {
      /* Non-empty stack - look for cards that can be placed on top */
      int top_stack = stack_get_card(s, count-1);
      for (j=0; j<7; j++)
	if (j != i)
	  for (k=stack_count_cards(maincells[j])-1; k>=0; k--)
	  {
	    int match = stack_get_card(maincells[j], k);
	    if (COLOR(match) != COLOR(top_stack)
		&& VALUE(match) == VALUE(top_stack)-1
		&& ! FACEDOWNP(match))
	      {
		/* Check if this is a pointless move */
		int is_pointless = 0;
		if (k > 0)  /* There's a card below */
		  {
		    int card_below = stack_get_card(maincells[j], k-1);
		    if (!FACEDOWNP(card_below)
			&& VALUE(card_below) == VALUE(top_stack)
			&& COLOR(card_below) != COLOR(match))
		      {
			/* Would be pointless unless card_below can go to foundation */
			if (!can_move_to_foundation(card_below))
			  is_pointless = 1;
		      }
		  }
		if (!is_pointless)
		  {
		    set_arrow(i, 1);
		    main_arrow_count++;
		    break;  /* Only count once per stack */
		  }
	      }
	  }
    }
  }

  /* Foundation arrows: only show if NO main stack arrows are showing */
  if (main_arrow_count == 0)
  {
    /* Check each foundation pile */
    for (i=0; i<4; i++)
    {
      int fc = stack_count_cards(outcells[i]);
      int target_value, target_suit;
      int found_move = 0;

      /* Determine what card we're looking for */
      if (fc == 0)
      {
	/* Empty foundation - looking for an Ace */
	target_value = ACE;
	target_suit = i;  /* Foundation 0=hearts, 1=diamonds, 2=clubs, 3=spades */
      }
      else
      {
	/* Get top card of foundation */
	int ftop = stack_get_card(outcells[i], fc-1);
	target_value = VALUE(ftop) + 1;
	target_suit = SUIT(ftop);
      }

      /* Check all 7 stacks for a matching top card */
      for (j=0; j<7 && !found_move; j++)
      {
	int sc = stack_count_cards(maincells[j]);
	if (sc > 0)
	{
	  int top_card = stack_get_card(maincells[j], sc-1);
	  if (!FACEDOWNP(top_card)
	      && VALUE(top_card) == target_value
	      && SUIT(top_card) == target_suit)
	  {
	    found_move = 1;
	  }
	}
      }

      set_foundation_arrow(i, found_move);
    }
  }
  else
  {
    /* Main arrows are showing, so clear all foundation arrows */
    clear_foundation_arrows();
  }
}

static void
start_again()
{
  int i, j;

  clear_arrows();

  for (i = 0; i < 4; i++)
    stack_flip_stack(outcells[i], deck);

  for (i = 0; i < 7; i++)
    stack_flip_stack(maincells[i], deck);

  stack_shuffle(deck);
  stack_shuffle(deck);
  stack_shuffle(deck);

  // Stack 0: 1 face up card
  stack_flip_card(deck, maincells[0]);

  // Stacks 1-6: i face down cards + 5 face up cards
  for (i = 1; i < 7; i++) {
    // Add i face down cards
    for (j = 0; j < i; j++)
      stack_move_cards(deck, stack_count_cards(deck)-1, maincells[i]);
    // Add 5 face up cards
    for (j = 0; j < 5; j++)
      stack_flip_card(deck, maincells[i]);
  }

  stack_undo_reset();
  set_arrows();
  score_increment_starts();
}


static void
init()
{
  int s, v;
  Picture *empty;

  // Load pictures first (needs QApplication to be initialized)
  xlogo = get_picture("xemboss");
  splash = get_picture("yukon");
  youwin = get_picture("youwin");
  youlose = get_picture("youlose");
  arrow = get_picture("thornq-arrow");
  no_arrow = get_picture("thornq-noarrow");

  stack_load_standard_deck();
  empty = get_picture("empty");

  xlogo_x = 3*M+2*W+W/2-xlogo->w/2;
  xlogo_y = M+H/2-xlogo->h/2;

  set_centered_pic(splash);

  for (s = 0; s < 4; s++) {
    outcells[s] = stack_create(4*W + 9*M + s*(W+M), M);
    stack_set_empty_picture(outcells[s], empty);
  }

  /* Move main stacks down to make room for foundation arrows */
  int arrow_space = arrow->h + 5;
  for (s = 0; s < 7; s++) {
    maincells[s] = stack_create(M+s*(M+W), H+2*M+arrow_space);
    stack_set_offset(maincells[s], STACK_OFFSET_DOWN);
  }

  /* Initialize foundation arrow positions */
  for (s = 0; s < 4; s++) {
    found_arrowsx[s] = 4*W + 9*M + s*(W+M) + W/2;
    found_arrow_ypos[s] = M + H + 2;
    found_arrowsy[s] = 0;  /* Start with arrows off */
  }

  deck = stack_create(10*W, 0);

  for (s = 0; s < 4; s++)
    for (v = ACE; v <= KING; v++)
      stack_add_card(deck, MAKE_CARD(s, v, FACEDOWN));

  score_init(xwin_get_argv0());
  start_again();
}


static void
redraw()
{
  if (xlogo_x || xlogo_y)
    put_picture(xlogo, xlogo_x, xlogo_y, 0, 0, xlogo->h, xlogo->w);
  stack_redraw();
  redraw_foundation_arrows();
  redraw_arrows();
}


static void
key(int k, int x, int y)
{

  Picture *cp = get_centered_pic();

  switch (k) {
  case 3: case 27: case 'q':
    exit(0);

  case KEY_F(1): case 'h':
    set_centered_pic(0);
    help("yukon.html");
    return;

  case KEY_F(2): case 'r':
    set_centered_pic(0);
    start_again();
    while (auto_move());
    return;

  case 8: case 127: case KEY_DELETE:
    set_centered_pic(0);
    clear_arrows();
    stack_undo();
    set_arrows();
    return;
  }

  if (cp == youlose || cp == youwin)
    {
      set_centered_pic(0);
      start_again();
      while (auto_move());
      return;
    }

  switch (k) {
  case 'a':
  case 'A':
    clear_arrows();
    clear_foundation_arrows();
    supress_arrows = !supress_arrows;
    set_arrows();

    break;

/*
    case ' ':
    double_click(x, y, 1);
    break;*/
  }
}


static char *
card_string(int c)
{
  static char names[5][5];
  static int n=0;
  static char suits[] = "HDCS";
  static char face[] = "A234567890JQK";

  n = (n+1)%5;
  sprintf(names[n], "%c%c", face[VALUE(c)], suits[SUIT(c)]);

  return names[n];
}


static Stack *src_stack = 0;
static int src_n = 0;

static Stack *dest_stack;
static int last_n = -1;

static int
n_droppable_s(Stack *dest_stack)
{
  int src_top, dest_top, src_count, dest_count, i, j;

  src_count = stack_count_cards(src_stack);
  dest_count = stack_count_cards(dest_stack);
  dest_top = stack_get_card(dest_stack, dest_count-1);
  src_top = stack_get_card(src_stack, src_count-1);

  if (dest_stack == src_stack)
    return src_n;

  /* Allow manual dragging to foundations (outcells) */
  for (i = 0; i < 4; i++)
    if (dest_stack == outcells[i]) {
      if (dest_count == 0 && VALUE(src_top) == ACE)
	return src_count - 1;
      if (dest_count > 0 && VALUE(src_top) == VALUE(dest_top) + 1
	  && SUIT(src_top) == SUIT(dest_top))
	return src_count - 1;
      return src_count;
    }

  for (i = 0; i < 7; i++)
    if (dest_stack == maincells[i])
      {
	/* move entire stack to empty slot */
	if (0 == src_n && 0 == dest_count)
	  return 0;

	if (dest_count == 0)
	  {
	    /* look for kings */
	    for (j=src_n; j<src_count; j++)
	      {
		src_top = stack_get_card(src_stack, j);
		if (!FACEDOWNP(src_top) && VALUE(src_top) == KING)
		  return j;
	      }
	    for (j=src_n-1; j>=0; j--)
	      {
		src_top = stack_get_card(src_stack, j);
		if (!FACEDOWNP(src_top) && VALUE(src_top) == KING)
		  return j;
	      }
	    return 0;
	  }
	else
	  {
	    /* Look for moveable cards */
	    /* First check if the card the user clicked (src_n) can be dropped */
	    src_top = stack_get_card(src_stack, src_n);
	    if (!FACEDOWNP(src_top)
		&& COLOR(src_top) != COLOR(dest_top)
		&& VALUE(src_top) == VALUE(dest_top)-1)
	      return src_n;

	    /* Otherwise, search entire stack for any valid card */
	    for (j=0; j<src_count; j++)
	      {
		src_top = stack_get_card(src_stack, j);
		if (!FACEDOWNP(src_top)
		    && COLOR(src_top) != COLOR(dest_top)
		    && VALUE(src_top) == VALUE(dest_top)-1)
		  return j;
	      }
	  }

	break;
      }

  return src_count;
}

static int
n_droppable(int x, int y)
{
  if (!src_stack)
    return 0;

  if (!stack_drag_find(x, y, &dest_stack)) {
    dest_stack = src_stack;

    return last_n != -1 ? last_n : src_n;
  }

  return n_droppable_s(dest_stack);
}


static int suit_values[4];  // Highest value on foundation for each suit
static int pile_for[4];
static int top_card[4];

static int
auto_move_stack(Stack *s)
{
  int i, c, n = stack_count_cards(s);
  if (n == 0)
    return 0;
  c = stack_get_card(s, n-1);
  if (FACEDOWNP(c))
  {
    stack_flip_card(s, s);
    c = stack_get_card(s, n-1);
    return 1;
  }
  if (VALUE(c) == ACE)
  {
    for (i=0; i<4; i++)
      if (stack_count_cards(outcells[i]) == 0)
      {
	stack_animate(s, outcells[i]);
	return 1;
      }
    return 0; /* can't happen? */
  }
  if (pile_for[SUIT(c)] == -1)
    return 0;

  /* For safety, check that both opposite-color suits have cards high enough
     that they can't be placed back on this card in the tableau */
  int my_color = COLOR(c);
  int safe = 1;

  /* Check both suits of opposite color */
  for (i = 0; i < 4; i++)
  {
    if (COLOR(MAKE_CARD(i, ACE, FACEUP)) != my_color)
    {
      /* This is an opposite-color suit - it needs VALUE-1 or higher */
      if (suit_values[i] < VALUE(c) - 1)
      {
#if DEBUG_YUKON
        printf("DEBUG: Card %d of suit %d UNSAFE: opposite suit %d only has %d (need %d)\n",
               VALUE(c), SUIT(c), i, suit_values[i], VALUE(c) - 1);
#endif
        safe = 0;
        break;
      }
    }
  }

  if (VALUE(c) == VALUE(top_card[pile_for[SUIT(c)]])+1 && safe)
  {
#if DEBUG_YUKON
    printf("DEBUG: AUTO-MOVING card %d of suit %d!\n", VALUE(c), SUIT(c));
#endif
    stack_animate(s, outcells[pile_for[SUIT(c)]]);
    return 1;
  }
  return 0;
}

static int
auto_move()
{
  int i, something_moved = 0;

  /* Initialize: all suits start with 0 (no cards) */
  memset(suit_values, 0, sizeof(suit_values));
  pile_for[0] = pile_for[1] = pile_for[2] = pile_for[3] = -1;
  memset(top_card, 0, sizeof(top_card));

  /* Scan foundations to find highest value for each suit */
  for (i=0; i<4; i++)
  {
    int c, n = stack_count_cards(outcells[i]);
    if (n == 0) continue;
    c = stack_get_card(outcells[i], n-1);
    top_card[i] = c;
    pile_for[SUIT(c)] = i;
    suit_values[SUIT(c)] = VALUE(c);
  }

#if DEBUG_YUKON
  printf("DEBUG: suit_values: [H]=%d [D]=%d [C]=%d [S]=%d\n",
         suit_values[0], suit_values[1], suit_values[2], suit_values[3]);
  printf("DEBUG: pile_for: [0]=%d [1]=%d [2]=%d [3]=%d\n",
         pile_for[0], pile_for[1], pile_for[2], pile_for[3]);
#endif

  for (i=0; i<7; i++)
    something_moved |= auto_move_stack(maincells[i]);

  return something_moved;
}


static void
check_for_end_of_game()
{
  int i, j, k, available_moves=0;

  set_arrows();

  if (stack_count_cards(outcells[0]) == 13 &&
      stack_count_cards(outcells[1]) == 13 &&
      stack_count_cards(outcells[2]) == 13 &&
      stack_count_cards(outcells[3]) == 13)
  {
    set_centered_pic(youwin);
    score_increment_wins();
    return;
  }

  if (hint_mode)
    printf("Available moves:\n");

  /* Check if any top cards can move to foundation */
  for (i=0; i<7; i++)
  {
    Stack *s = maincells[i];
    int sc = stack_count_cards(s);
    if (sc > 0)
    {
      int topcard = stack_get_card(s, sc-1);
      if (!FACEDOWNP(topcard) && can_move_to_foundation(topcard))
      {
	if (!hint_mode)
	  return;
	printf("  move %c%c from stack %d to foundation\n",
	       values[VALUE(topcard)], suits[SUIT(topcard)], i+1);
	available_moves++;
      }
    }
  }

  /* Check moves between tableau stacks */
  for (i=0; i<7; i++)
  {
    Stack *d = maincells[i];
    int dc = stack_count_cards(d);
    int topcard = stack_get_card(d, dc-1);
    for (j=0; j<7; j++)
      if (j != i)
      {
	Stack *s = maincells[j];
	for (k=stack_count_cards(s)-1; k>=0; k--)
	{
	  int one = stack_get_card(s, k);
	  if (!FACEDOWNP(one))
	  {
	    if (dc == 0)
	    {
	      if (VALUE(one) == KING && k > 0)
	      {
		if (!hint_mode)
		  return;
		printf("  Move K%c from stack %d to stack %d\n",
		       suits[SUIT(one)], j+1, i+1);
		available_moves++;
	      }
	    }
	    else
	    {
	      if (COLOR(one) != COLOR(topcard)
		  && VALUE(one) == VALUE(topcard)-1)
	      {
		/* Check if this is a pointless move */
		int is_pointless = 0;
		if (k > 0)  /* There's a card below */
		  {
		    int card_below = stack_get_card(s, k-1);
		    if (!FACEDOWNP(card_below)
			&& VALUE(card_below) == VALUE(topcard)
			&& COLOR(card_below) != COLOR(one))
		      {
			/* Would be pointless unless card_below can go to foundation */
			if (!can_move_to_foundation(card_below))
			  is_pointless = 1;
		      }
		  }
		if (!is_pointless)
		  {
		    if (!hint_mode)
		      return;
		    printf("  move %c%c from stack %d onto stack %d\n",
			   values[VALUE(one)], suits[SUIT(one)],
			   j+1, i+1);
		    available_moves++;
		  }
	      }
	    }
	  }
	}
      }
  }
  if (!available_moves)
    set_centered_pic(youlose);
}

static void
click(int x, int y, int b)
{
  int c, f;

  Picture *cp = get_centered_pic();

  if (cp == youlose || cp == youwin)
    {
      set_centered_pic(0);
      start_again();
      while (auto_move());
      return;
    }

  if (cp == splash) {
    set_centered_pic(0);

    while (auto_move())
      ;

    return;
  }

  if (b == 4)
  {
    // Button 4: Undo (same as backspace key)
    set_centered_pic(0);
    clear_arrows();
    stack_undo();
    set_arrows();
    return;
  }
  if (b == 5)
  {
    // Button 5: Restart (same as 'r' key)
    set_centered_pic(0);
    start_again();
    while (auto_move());
    return;
  }

  src_stack = 0;
  if (!(f = stack_find(x, y, &src_stack, &src_n)))
    return;

  if (FACEDOWNP(stack_get_card(src_stack, stack_count_cards(src_stack)-1))
      == FACEDOWN) {
    stack_flip_card(src_stack, src_stack);
    return;
  }

  if (b == 2 || b == 3) {
    stack_peek_card(src_stack, src_n, 1);

    return;
  }

  last_n = -1;

  for (c=0; c<4; c++)
    if (src_stack == outcells[c])
      return;

  if (f && stack_count_cards(src_stack) > 0)
  {
    clear_arrows();
    stack_begin_drag(src_stack, src_n, x, y);
  }
  else
    src_stack = 0;
}

static int
try_moving_to(int i)
{
  int c = stack_get_card(src_stack, src_n);
  for (i=0; i<7; i++)
    if (maincells[i] != src_stack)
      {
	int j = stack_count_cards(maincells[i]) - 1;
	int f = stack_get_card(maincells[i], j);
	if (COLOR(f) != COLOR(c)
	    && VALUE(c) == VALUE(f) - 1
	    && ! FACEDOWNP(c)) {
	  clear_arrows();
	  stack_move_cards(src_stack, src_n, maincells[i]);
	  while (auto_move()) ;
	  set_arrows();
	  check_for_end_of_game();
	  return 1;
	}
      }
  return 0;
}

static int
try_moving_from(int i)
{
  int c = stack_get_card(src_stack, stack_count_cards(src_stack)-1);
  for (i=0; i<7; i++)
    if (maincells[i] != src_stack)
      {
	int j, n = stack_count_cards(maincells[i]);
	for (j=0; j<n; j++)
	  {
	    int f = stack_get_card(maincells[i], j);
	    if (COLOR(f) != COLOR(c)
		&& VALUE(f) == VALUE(c) - 1
		&& ! FACEDOWNP(f)) {
	      clear_arrows();
	      if (j == n-1)
		stack_animate(maincells[i], src_stack);
	      else
		stack_move_cards(maincells[i], j, src_stack);
	      while (auto_move()) ;
	      set_arrows();
	      check_for_end_of_game();
	      return 1;
	    }
	  }
      }
  return 0;
}

static void
double_click(int x, int y, int b)
{
  int c, f, i, j;

  src_stack = 0;

  if (!(stack_find(x, y, &src_stack, &src_n)))
    return;

  if (b > 1)
    return;

  /* verify that it's a main stack we double clicked on */
  for (i=0; i<7; i++)
    if (maincells[i] == src_stack)
      break;
  if (i == 7)
    return;

  if (stack_count_cards(src_stack) == 0)
    {
      /* find a king, and move it */
      for (i=0; i<7; i++)
	for (j=stack_count_cards(maincells[i])-1; j>0; j--)
	  {
	    c=stack_get_card(maincells[i], j);
	    if (!FACEDOWNP(c) && VALUE(c) == KING)
	      {
		clear_arrows();
		stack_move_cards(maincells[i], j, src_stack);
		while (auto_move()) ;
		set_arrows();
		check_for_end_of_game();
		return;
	      }
	  }
    }

  /* Check if clicked card is a king - try to move to empty stack */
  c = stack_get_card(src_stack, src_n);
  if (!FACEDOWNP(c) && VALUE(c) == KING)
    {
      for (i=0; i<7; i++)
	if (maincells[i] != src_stack && stack_count_cards(maincells[i]) == 0)
	  {
	    clear_arrows();
	    stack_move_cards(src_stack, src_n, maincells[i]);
	    while (auto_move()) ;
	    set_arrows();
	    check_for_end_of_game();
	    return;
	  }
    }

  /* clicked on top card or below it */
  if (src_n >= stack_count_cards(src_stack) - 1 || src_n < 0)
    if (try_moving_from(i))
      return;

  /* clicked on underlying card */
  if (try_moving_to(i))
    return;

  /* this didn't make sense to me - DJ */
#if 0
  /* oh well, try top card anyway */
  try_moving_from(i);
#endif
}

static void
drag(int x, int y, int b)
{
  if (b > 1)
    return;

  last_n = n_droppable(x, y);

  stack_continue_drag(last_n, x, y);
}

static void
drop(int x, int y, int b)
{
  last_n = n_droppable(x, y); /* also sets dest_stack */

  if (b > 1) {
    if (src_stack)
      stack_peek_card(src_stack, src_n, 0);

    return;
  }

  clear_arrows();
  stack_drop(dest_stack, last_n);

  while (auto_move())
    ;

  set_arrows();
  check_for_end_of_game();
}

static FunctionMapping fmap[] = {
  { "click", (void *)click },
  { "double_click", (void *)double_click },
  { "drag", (void *)drag },
  { "drop", (void *)drop },
  { "init", (void *)init },
  { "key", (void *)key },
  { "redraw", (void *)redraw },
  { "resize", (void *)resize },
  { 0, 0 }
};

int
main(int argc, char **argv)
{
  table_height = 600;
  register_imagelib(appimglib_imagelib);
  init_ace(argc, argv, fmap);

  if (table_width == 0 )
    {
      table_width = WIN_W;
    }
  init_table(table_width, table_height);
  table_loop();

  return 0;
}
