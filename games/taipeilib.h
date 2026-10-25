#include "taipei.h"

extern unsigned char grid[GRID_SX][GRID_SY][GRID_SZ];

extern int layer;
extern char *filename;
extern int display_scale;

void load(int complain);
void invalidate_tile(int x, int y, int z);
void prescale_spritesheet();

#define gx2x(x,z) (((x)*GRID_DX+(z)*GRID_DZ+MARGIN) * display_scale)
#define gy2y(y,z) (((y)*GRID_DY+(GRID_SZ-(z))*GRID_DZ+MARGIN) * display_scale)
#define x2gx(x,z) (((x) / display_scale - (z)*GRID_DZ - MARGIN) / GRID_DX)
#define y2gy(y,z) (((y) / display_scale - (GRID_SZ-(z))*GRID_DZ - MARGIN) / GRID_DY)

extern struct image_list tile_images[];
