/* The Ace of Penguins - qtaipedit.cpp
   Qt5/Qt6 implementation of Taipei layout editor
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

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QIcon>
#include <cstdio>
#include <cstring>

extern "C" {
#include "imagelib.h"
#include "cards.h"
#include "taipeilib.h"

// Help system from qhelp.cpp
void help(char *filename);

// Card synthesis image library
extern image_list card_images[];

// Global variables from taipeilib.c
extern char *filename;
extern int layer;
}

// Qt Pixmap type for images (must match definition in qwin.cpp)
typedef struct {
    QPixmap *image_pixmap;
    QPixmap *image_mask;
    QPixmap *rotated_pixmap;
    QPixmap *rotated_mask;
    QPixmap *inverted_pixmap;
} qt_pixels_type;

// Grid and tile globals from taipeilib
extern unsigned char grid[GRID_SX][GRID_SY][GRID_SZ];

// Display scale for coordinate macros (qtaipedit always uses 1x)
int display_scale = 1;

static Picture *tpe_bs, *tpe_bu, *tpe_bt;
static Picture *tiles[9];
static Picture *splash;
static int tile_count = 0;
static bool show_splash = true;

static int lbx, lby, lbw, lbh;
extern int table_width, table_height;

// Forward declarations
static void show_tile_count();

class TaipeditWindow : public QWidget {
public:
    TaipeditWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Ace of Penguins - Taipei Layout Editor");
        setFocusPolicy(Qt::StrongFocus);

        // Set window icon to synthesized "as" (ace of spades) card
        Picture *icon_pic = get_picture(const_cast<char*>("as"));
        if (icon_pic && icon_pic->w > 0) {
            image *icon_img = reinterpret_cast<image*>(icon_pic);
            if (icon_img && icon_img->pixels) {
                qt_pixels_type *icon_qp = reinterpret_cast<qt_pixels_type*>(icon_img->pixels);
                if (icon_qp && icon_qp->image_pixmap) {
                    setWindowIcon(QIcon(*icon_qp->image_pixmap));
                }
            }
        }

        // Set window size
        resize(table_width, table_height);
        setMinimumSize(table_width, table_height);

        // Set green background
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(0x00, 0x66, 0x00));
        setAutoFillBackground(true);
        setPalette(palette);
    }

protected:
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);

        // Draw grid lines
        int dy = GRID_DZ - 1;
        painter.setPen(Qt::black);

        // Vertical grid lines
        for (int x = 0; x <= GRID_SX; x++) {
            int x1 = gx2x(x, 0);
            int y1 = gy2y(0, 0) + dy;
            int y2 = gy2y(GRID_SY, 0) + dy;
            painter.drawLine(x1, y1, x1, y2);
        }

        // Horizontal grid lines
        for (int y = 0; y <= GRID_SY; y++) {
            int x1 = gx2x(0, 0);
            int x2 = gx2x(GRID_SX, 0);
            int y1 = gy2y(y, 0) + dy;
            painter.drawLine(x1, y1, x2, y1);
        }

        // Diagonal lines
        painter.drawLine(gx2x(0, 0), gy2y(0, 0) + dy,
                        gx2x(GRID_SX, 0), gy2y(GRID_SY, 0) + dy);
        painter.drawLine(gx2x(0, 0), gy2y(GRID_SY, 0) + dy,
                        gx2x(GRID_SX, 0), gy2y(0, 0) + dy);

        // Draw tile count above layer buttons
        char tmp[10];
        snprintf(tmp, sizeof(tmp), "%5d%c", tile_count, tile_count % 4 ? '!' : ' ');

        // Use same fixed-width font as taipei game (from qwin.cpp text())
        QFont font("Courier", 12);
        painter.setFont(font);
        painter.setPen(Qt::white);

        // Draw text aligned with left edge of button column
        // Adjust x position to align with button left edge
        painter.drawText(lbx - 20, lby - lbh * GRID_SZ + 2, tmp);

        // Draw layer buttons
        for (int z = 0; z < GRID_SZ; z++) {
            Picture *p;
            if (z < layer)
                p = tpe_bs;
            else if (z == layer)
                p = tpe_bt;
            else
                p = tpe_bu;

            drawPicture(&painter, p, lbx, lby - z * lbh);
        }

        // Draw tiles
        for (int z = 0; z <= layer; z++) {
            for (int x = GRID_SX - 1; x >= 0; x--) {
                for (int y = 0; x + y < GRID_SX && y < GRID_SY; y++) {
                    if (grid[x + y][y][z]) {
                        drawPicture(&painter, tiles[z], gx2x(x + y, z), gy2y(y, z));
                    }
                }
            }
            for (int y = 0; y < GRID_SY; y++) {
                for (int x = 0; x + y < GRID_SY && x < GRID_SX; x++) {
                    if (grid[x][x + y][z]) {
                        drawPicture(&painter, tiles[z], gx2x(x, z), gy2y(x + y, z));
                    }
                }
            }
        }

        // Draw splash screen on top if active
        if (show_splash && splash) {
            int x = (width() - splash->w) / 2;
            int y = (height() - splash->h) / 2;
            drawPicture(&painter, splash, x, y);
        }
    }

    void mousePressEvent(QMouseEvent *event) override {
        // Dismiss splash screen on any click
        if (show_splash) {
            show_splash = false;
            update();
            return;
        }

        int x = event->pos().x();
        int y = event->pos().y();
        int button = (event->button() == Qt::LeftButton) ? 1 : 3;

        handleClick(x, y, button);
        update();
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (event->buttons() & (Qt::LeftButton | Qt::RightButton)) {
            int x = event->pos().x();
            int y = event->pos().y();
            int button = (event->buttons() & Qt::LeftButton) ? 1 : 3;

            handleClick(x, y, button);
            update();
        }
    }

    void keyPressEvent(QKeyEvent *event) override {
        // Dismiss splash screen on any key
        if (show_splash) {
            show_splash = false;
            update();
            return;
        }

        int k = event->key();

        // Handle quit
        if (k == Qt::Key_Q || k == Qt::Key_Escape) {
            close();
            return;
        }

        // Handle help
        if (k == Qt::Key_H || k == Qt::Key_F1) {
            help(const_cast<char*>("taipedit.html"));
            return;
        }

        // Handle save/load
        if (k == Qt::Key_S) {
            saveLayout();
            return;
        }

        if (k == Qt::Key_L) {
            loadLayout();
            return;
        }

        // Handle layer selection (1-5)
        if (k >= Qt::Key_1 && k <= Qt::Key_9) {
            int new_layer = k - Qt::Key_1;
            if (new_layer < GRID_SZ && layer != new_layer) {
                layer = new_layer;
                update();
            }
            return;
        }

        // Handle arrow keys for shifting
        switch (k) {
            case Qt::Key_Up:
                if (checkEmpty(0, GRID_SX-1, 0, 0, 0, GRID_SZ-1))
                    shiftGrid(0, 1, 0);
                break;
            case Qt::Key_Down:
                if (checkEmpty(0, GRID_SX-1, GRID_SY-2, GRID_SY-1, 0, GRID_SZ-1))
                    shiftGrid(0, -1, 0);
                break;
            case Qt::Key_Left:
                if (checkEmpty(0, 0, 0, GRID_SY-1, 0, GRID_SZ-1))
                    shiftGrid(1, 0, 0);
                break;
            case Qt::Key_Right:
                if (checkEmpty(GRID_SX-2, GRID_SX-1, 0, GRID_SY-1, 0, GRID_SZ-1))
                    shiftGrid(-1, 0, 0);
                break;
            case Qt::Key_PageUp:
                if (checkEmpty(0, GRID_SX-1, 0, GRID_SY-1, GRID_SZ-1, GRID_SZ-1))
                    shiftGrid(0, 0, -1);
                break;
            case Qt::Key_PageDown:
                if (checkEmpty(0, GRID_SX-1, 0, GRID_SY-1, 0, 0))
                    shiftGrid(0, 0, 1);
                break;
            case Qt::Key_Home:
                autoCenter();
                break;
        }
    }

private:
    void drawPicture(QPainter *painter, Picture *pic, int x, int y) {
        if (!pic) return;

        image *img = reinterpret_cast<image*>(pic);
        if (!img) return;

        // If image has a synthesis function, call it to generate the graphics
        if (img->synth_func) {
            img->synth_func(img);
        }

        if (!img->pixels) return;

        qt_pixels_type *qp = reinterpret_cast<qt_pixels_type*>(img->pixels);
        if (!qp || !qp->image_pixmap) return;

        painter->drawPixmap(x, y, *qp->image_pixmap);
    }

    bool checkLayerButtons(int x, int y) {
        if (x >= lbx) {
            for (int z = 0; z < GRID_SZ; z++) {
                if (y >= lby - z * lbh && y < lby - z * lbh + lbh) {
                    if (layer != z) {
                        layer = z;
                        update();
                    }
                    return true;
                }
            }
        }
        return false;
    }

    void addTile(int x, int y) {
        if (tile_count >= 144) return;

        int nx = x2gx(x - GRID_DX/2, layer);
        int ny = y2gy(y - GRID_DY/2, layer);
        int nz = layer;

        if (nx < 0 || nx >= GRID_SX-1 ||
            ny < 0 || ny >= GRID_SY-1 ||
            nz < 0 || nz >= GRID_SZ)
            return;

        // Check for overlapping tiles
        for (int cx = nx-1; cx <= nx+1; cx++) {
            if (cx >= 0 && cx < GRID_SX) {
                for (int cy = ny-1; cy <= ny+1; cy++) {
                    if (cy >= 0 && cy < GRID_SY) {
                        if (grid[cx][cy][nz])
                            return;
                    }
                }
            }
        }

        grid[nx][ny][nz] = 1;
        tile_count++;
        show_tile_count();
    }

    void removeTile(int x, int y) {
        for (int nx = 0; nx < GRID_SX; nx++) {
            if (x >= gx2x(nx, layer) && x < gx2x(nx, layer) + TILE_SX) {
                for (int ny = 0; ny < GRID_SY; ny++) {
                    if (y >= gy2y(ny, layer) && y < gy2y(ny, layer) + TILE_SY) {
                        if (grid[nx][ny][layer]) {
                            grid[nx][ny][layer] = 0;
                            tile_count--;
                            show_tile_count();
                        }
                    }
                }
            }
        }
    }

    void handleClick(int x, int y, int button) {
        if (checkLayerButtons(x, y))
            return;

        if (button == 1)
            addTile(x, y);
        else
            removeTile(x, y);
    }

    bool checkEmpty(int x1, int x2, int y1, int y2, int z1, int z2) {
        for (int x = x1; x <= x2; x++) {
            for (int y = y1; y <= y2; y++) {
                for (int z = z1; z <= z2; z++) {
                    if (grid[x][y][z])
                        return false;
                }
            }
        }
        return true;
    }

    void shiftGrid(int dx, int dy, int dz) {
        int x1 = 0, x2 = GRID_SX-1, y1 = 0, y2 = GRID_SY-1, z1 = 0, z2 = GRID_SZ-1;
        unsigned char temp[GRID_SX][GRID_SY][GRID_SZ];
        memset(temp, 0, sizeof(temp));

        if (dx > 0) x2 -= dx;
        if (dx < 0) x1 -= dx;
        if (dy > 0) y2 -= dy;
        if (dy < 0) y1 -= dy;
        if (dz > 0) z2 -= dz;
        if (dz < 0) z1 -= dz;

        for (int x = x1; x <= x2; x++) {
            for (int y = y1; y <= y2; y++) {
                for (int z = z1; z <= z2; z++) {
                    temp[x][y][z] = grid[x+dx][y+dy][z+dz];
                }
            }
        }

        memcpy(grid, temp, sizeof(temp));
        update();
    }

    void autoCenter() {
        int minx = GRID_SX, maxx = 0, miny = GRID_SY, maxy = 0, minz = GRID_SZ, maxz = 0;

        for (int x = 0; x < GRID_SX; x++) {
            for (int y = 0; y < GRID_SY; y++) {
                for (int z = 0; z < GRID_SZ; z++) {
                    if (grid[x][y][z]) {
                        if (minx > x) minx = x;
                        if (maxx < x) maxx = x;
                        if (miny > y) miny = y;
                        if (maxy < y) maxy = y;
                        if (minz > z) minz = z;
                        if (maxz < z) maxz = z;
                    }
                }
            }
        }

        shiftGrid((minx + maxx - GRID_SX + 2)/2,
                 (miny + maxy - GRID_SY + 2)/2,
                 minz);
    }

    void saveLayout() {
        // Determine starting directory
        // Windows: Current directory (executable location)
        // Linux: User's home directory
#ifdef _WIN32
        QString start_dir = QDir::currentPath();
#else
        QString start_dir = QDir::homePath();
#endif

        // Create dialog explicitly to set font size
        QFileDialog dialog(this, "Save Taipei Layout", start_dir,
            "Taipei Layouts (*.tp);;All Files (*)");
        dialog.setFileMode(QFileDialog::AnyFile);
        dialog.setAcceptMode(QFileDialog::AcceptSave);
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

        if (fname.isEmpty()) return;

        // Ensure .tp extension
        if (!fname.endsWith(".tp", Qt::CaseInsensitive)) {
            fname += ".tp";
        }

        FILE *f = fopen(fname.toUtf8().constData(), "w");
        if (!f) {
            QMessageBox::warning(this, "Error",
                QString("Could not save file: %1").arg(fname));
            return;
        }

        // Write layout file (format from original taipedit.c: -x,y,z,)
        for (int z = 0; z < GRID_SZ; z++) {
            for (int y = 0; y < GRID_SY; y++) {
                bool samey = false;
                int w = 0;
                for (int x = 0; x < GRID_SX; x++) {
                    if (grid[x][y][z]) {
                        if (w > 20) {
                            w = 0;
                            fputc('\n', f);
                        }
                        if (samey) {
                            fprintf(f, "%d,", x);
                            w += 1;
                        } else {
                            fprintf(f, "-%d,%d,%d,", x, y, z);
                            samey = true;
                            w += 3;
                        }
                    }
                }
            }
        }

        fclose(f);
        QMessageBox::information(this, "Saved",
            QString("Layout saved to: %1").arg(fname));
    }

    void loadLayout() {
        // Determine starting directory
        // Windows: Current directory (executable location)
        // Linux: User's home directory
#ifdef _WIN32
        QString start_dir = QDir::currentPath();
#else
        QString start_dir = QDir::homePath();
#endif

        // Create dialog explicitly to set font size
        QFileDialog dialog(this, "Load Taipei Layout", start_dir,
            "Taipei Layouts (*.tp)");
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

        if (fname.isEmpty()) return;

        // Free old filename if it exists
        if (filename) {
            free(filename);
            filename = nullptr;
        }

        // Allocate and set filename for taipeilib's load() function
        QByteArray utf8 = fname.toUtf8();
        filename = (char*)malloc(utf8.length() + 1);
        strcpy(filename, utf8.constData());

        // Use taipeilib's load() function (complain=1 to show errors)
        load(1);

        // Recalculate tile count
        tile_count = 0;
        for (int x = 0; x < GRID_SX; x++)
            for (int y = 0; y < GRID_SY; y++)
                for (int z = 0; z < GRID_SZ; z++)
                    if (grid[x][y][z])
                        tile_count++;

        show_tile_count();
        update();
    }
};

static TaipeditWindow *window = nullptr;

// Global qt_window pointer from qwin.cpp (needed for help system)
extern QWidget *qt_window;

static void show_tile_count() {
    // Recalculate tile count
    tile_count = 0;
    for (int x = 0; x < GRID_SX; x++)
        for (int y = 0; y < GRID_SY; y++)
            for (int z = 0; z < GRID_SZ; z++)
                if (grid[x][y][z])
                    tile_count++;

    if (window) {
        window->update();
    }
}

int main(int argc, char **argv) {
    // Suppress Qt platform warnings
    qputenv("QT_LOGGING_RULES", "qt.qpa.xcb=false");

    QApplication app(argc, argv);

    // Initialize resources
    Q_INIT_RESOURCE(qtaipedit);
    //CHUCK I don't think I need the line below
    //Q_INIT_RESOURCE(common);

    // Initialize card library
    register_imagelib(appimglib_imagelib);
    register_imagelib(tile_images);

    // Register card synthesis library for help system
    register_imagelib(card_images);

    // Set up table dimensions
    table_width = 640;
    table_height = 325;

    // Initialize grid
    memset(grid, 0, sizeof(grid));
    tile_count = 0;
    layer = 0;

    // Load images
    splash = get_picture(const_cast<char*>("taipedit"));

    char tmp[20];
    for (int x = 1; x < 9; x++) {
        sprintf(tmp, "n%d", x);
        // Request tiles at native 39x39 size (not default card size 73x97)
        tiles[x-1] = (Picture *)get_image(tmp, 39, 39, 0);
    }

    tpe_bs = get_picture(const_cast<char*>("taipedit-bs"));
    tpe_bu = get_picture(const_cast<char*>("taipedit-bu"));
    tpe_bt = get_picture(const_cast<char*>("taipedit-bt"));

    // Calculate layer button position
    lbw = tpe_bs->w;
    lbh = tpe_bs->h;
    lbx = table_width - lbw - 5;
    // Align bottom button with bottom of grid
    lby = gy2y(GRID_SY, 0) + GRID_DZ - 1 - lbh;

    // Create and show window
    window = new TaipeditWindow();
    window->show();

    // Set global qt_window pointer so help system can create overlay
    qt_window = window;

    return app.exec();
}
