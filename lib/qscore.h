/* The Ace of Penguins - qscore.h
   Copyright (C) 2026 Qt Port

   Game scorekeeping system using Qt XML handling
*/

#ifndef QSCORE_H
#define QSCORE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize scorekeeping system with game name from argv[0]
   Should be called during xwin_init() */
void score_init(const char *argv0);

/* Increment the number of times this game has been started */
void score_increment_starts(void);

/* Decrement the number of times this game has been started */
void score_decrement_starts(void);

/* Increment the number of times this game has been won */
void score_increment_wins(void);

/* Get current statistics for this game
   Returns 0 on success, -1 on error */
int score_get_stats(int *starts, int *wins);

#ifdef __cplusplus
}
#endif

#endif /* QSCORE_H */
