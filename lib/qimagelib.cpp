/* The Ace of Penguins - qimagelib.cpp
   Qt5 implementation of imagelib.c image loading/management
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

#include <QImage>
#include <QPixmap>
#include <QPainter>
#include <QString>
#include <QFile>
#include <QWidget>
#include <QHash>
#include <cstdio>
#include <cstring>
#include <cstdlib>

extern "C" {
#include "imagelib.h"
#include "cards.h"

/* Empty image library - game-specific images are now found directly
   in games/ folder via updated search paths. This array is kept empty
   to maintain the symbol but does nothing. */
image_list appimglib_imagelib[] = {
    { 0 } /* Null terminator */
};
}

// Debug mode
#define DEBUG_IMGLIB 0

// PERFORMANCE: Cache common colors to avoid repeated QColor construction
static const QColor table_green(0x00, 0x66, 0x00);   // RGB(0, 102, 0)

// PERFORMANCE: Cache loaded PNG files to avoid repeated decompression
// Key format: "name:size" (e.g., "suits:9", "a-k:2", "solitaire:0")
// Avoids reloading and decompressing the same PNG multiple times
static QHash<QString, QPixmap*> png_cache;

// PERFORMANCE: Reuse QPainter within batched operations to avoid repeated construction/destruction
// Created on first draw operation, destroyed on flush()
static QPainter *batch_painter = nullptr;
static QPixmap *batch_target = nullptr;

// Qt Pixmap type for images
typedef struct {
    QPixmap *pixmap;        // Main image
    QPixmap *mask;          // Alpha mask (if needed)
    QPixmap *rotated;       // Cached rotated version
    QPixmap *inverted;      // Cached inverted version
} qt_pixels_type;

// ============================================================================
// GLOBAL STATE
// ============================================================================

image *display_image = nullptr;

static image_list *registered_lists = nullptr;
static int preferred_display_type = TABLE_COLOR;

// Clipping region from qwin.cpp
extern QRect clip_rect;
extern bool clip_enabled;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

// Get or create the batch painter for a target pixmap
// PERFORMANCE: Reuses painter across multiple draw operations until flush()
// Non-static so it can be called from qwin.cpp
QPainter* get_batch_painter(QPixmap *target_pixmap)
{
    // Safety check: don't create painter for null pixmap
    if (!target_pixmap) {
        return nullptr;
    }

    if (batch_painter && batch_target == target_pixmap) {
        // Painter already active for this target
        return batch_painter;
    }

    // Different target or no painter - end current batch and start new one
    if (batch_painter) {
        delete batch_painter;
        batch_painter = nullptr;
        batch_target = nullptr;
    }

    // Create new painter for this target
    batch_painter = new QPainter(target_pixmap);
    batch_target = target_pixmap;
    return batch_painter;
}

// End the current batch (called from flush/flushsync in qwin.cpp)
extern "C" void end_paint_batch()
{
    if (batch_painter) {
        delete batch_painter;
        batch_painter = nullptr;
        batch_target = nullptr;
    }
}

// Create Qt pixels structure for an image
static qt_pixels_type *create_qt_pixels(QPixmap *pixmap)
{
    qt_pixels_type *qp = (qt_pixels_type *)malloc(sizeof(qt_pixels_type));
    qp->pixmap = pixmap;
    qp->mask = nullptr;
    qp->rotated = nullptr;
    qp->inverted = nullptr;
    return qp;
}

// Load PNG file and create QPixmap
// preferred_size: suggested size for multi-size images (0 = no preference)
static QPixmap *load_png_file(const char *name, int preferred_size)
{
#if DEBUG_IMGLIB
                printf("load_png_file attempting to load %s preferred size %i\n", name, preferred_size);
#endif

    // PERFORMANCE: Check cache first to avoid reloading/decompressing
    QString cache_key = QString("%1:%2").arg(name).arg(preferred_size);
    if (png_cache.contains(cache_key)) {
#if DEBUG_IMGLIB
        printf("PNG CACHE HIT: %s (avoiding reload/decompress)\n", cache_key.toUtf8().constData());
#endif
        return png_cache.value(cache_key);
    }

    // STEP 1: Try Qt embedded resources first (compiled-in resources)
    // Check game-specific resources first, then common resources
    const char *resource_prefixes[] = {
        ":/images/game",    // Game-specific resources (golf-arrow.png, solitaire.png, etc.)
        ":/images/png",     // Common resources (a-k.1.png, suits.png, etc.)
        nullptr
    };

    for (int i = 0; resource_prefixes[i]; i++) {
        QString resource_path = QString("%1/%2.png").arg(resource_prefixes[i]).arg(name);
        if (QFile::exists(resource_path)) {
            QPixmap *pixmap = new QPixmap(resource_path);
            if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                printf("Loaded image from resource: %s (%dx%d)\n", resource_path.toUtf8().constData(), pixmap->width(), pixmap->height());
#endif
                // Cache for future use
                png_cache.insert(cache_key, pixmap);
                return pixmap;
            }
            delete pixmap;
        }
    }

    // STEP 2: Try multiple file system search paths (fallback for development/modding)
    // Check game-specific folders first, then shared lib/png folder
    const char *search_paths[] = {
        "games/%s.png",           // Game-specific images (solitaire.png, etc.)
        "../games/%s.png",
        "../../games/%s.png",
        "lib/png/%s.png",         // Shared card/suit images
        "../lib/png/%s.png",
        "../../lib/png/%s.png",
        "png/%s.png",
        "%s.png",
        nullptr
    };

    // First try exact name (no size suffix)
    for (int i = 0; search_paths[i]; i++) {
        char path[256];
        snprintf(path, sizeof(path), search_paths[i], name);

        if (QFile::exists(path)) {
            QPixmap *pixmap = new QPixmap(path);
            if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                printf("Loaded image from file: %s (%dx%d)\n", path, pixmap->width(), pixmap->height());
#endif
                // Cache for future use
                png_cache.insert(cache_key, pixmap);
                return pixmap;
            }
            delete pixmap;
        }
    }

    // Try size-suffixed versions (e.g., suits.9.png, suits.13.png, etc.)
    // Available sizes in the original
    const int sizes[] = {9, 13, 26, 39, 2, 25, 50, 0};

    // If a preferred size was given, try it first
    if (preferred_size > 0) {
        // Try resources with preferred size (game-specific first, then common)
        for (int i = 0; resource_prefixes[i]; i++) {
            QString resource_path_sized = QString("%1/%2.%3.png").arg(resource_prefixes[i]).arg(name).arg(preferred_size);
            if (QFile::exists(resource_path_sized)) {
                QPixmap *pixmap = new QPixmap(resource_path_sized);
                if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                    printf("Loaded image from resource: %s (%dx%d) [preferred size]\n",
                           resource_path_sized.toUtf8().constData(), pixmap->width(), pixmap->height());
#endif
                    // Cache for future use
                    png_cache.insert(cache_key, pixmap);
                    return pixmap;
                }
                delete pixmap;
            }
        }

        // Try file system with preferred size
        for (int i = 0; search_paths[i]; i++) {
            char namewithsize[256];
            snprintf(namewithsize, sizeof(namewithsize), "%s.%d", name, preferred_size);

            char path[256];
            snprintf(path, sizeof(path), search_paths[i], namewithsize);

            if (QFile::exists(path)) {
                QPixmap *pixmap = new QPixmap(path);
                if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                    printf("Loaded image from file: %s (%dx%d) [preferred size]\n", path, pixmap->width(), pixmap->height());
#endif
                    // Cache for future use
                    png_cache.insert(cache_key, pixmap);
                    return pixmap;
                }
                delete pixmap;
            }
        }
    }

    // Fall back to trying all sizes
    for (int s = 0; sizes[s] != 0; s++) {
        // Skip the preferred size if we already tried it
        if (sizes[s] == preferred_size) continue;

        // Try resources with this size (game-specific first, then common)
        for (int i = 0; resource_prefixes[i]; i++) {
            QString resource_path_sized = QString("%1/%2.%3.png").arg(resource_prefixes[i]).arg(name).arg(sizes[s]);
            if (QFile::exists(resource_path_sized)) {
                QPixmap *pixmap = new QPixmap(resource_path_sized);
                if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                    printf("Loaded image from resource: %s (%dx%d) [fallback size]\n",
                           resource_path_sized.toUtf8().constData(), pixmap->width(), pixmap->height());
#endif
                    // Cache for future use
                    png_cache.insert(cache_key, pixmap);
                    return pixmap;
                }
                delete pixmap;
            }
        }

        // Try file system with this size
        for (int i = 0; search_paths[i]; i++) {
            char namewithsize[256];
            snprintf(namewithsize, sizeof(namewithsize), "%s.%d", name, sizes[s]);

            char path[256];
            snprintf(path, sizeof(path), search_paths[i], namewithsize);

            if (QFile::exists(path)) {
                QPixmap *pixmap = new QPixmap(path);
                if (!pixmap->isNull()) {
#if DEBUG_IMGLIB
                    printf("Loaded image from file: %s (%dx%d) [fallback size]\n", path, pixmap->width(), pixmap->height());
#endif
                    // Cache for future use
                    png_cache.insert(cache_key, pixmap);
                    return pixmap;
                }
                delete pixmap;
            }
        }
    }

#if DEBUG_IMGLIB
    printf("WARNING: Image not found: %s (tried base name and size suffixes)\n", name);
#endif
    return nullptr;
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

extern "C" int register_imagelib(image_list *images)
{
#if DEBUG_IMGLIB
    printf("register_imagelib(images=%p)\n", images);
#endif

    // Add to linked list of registered image libraries
    if (!images) return 0;

    image_list *list = images;
    while (list && list->name) {
        list->next = registered_lists;
        registered_lists = list;
#if DEBUG_IMGLIB
        printf("  Registered image list: %s\n", list->name);
#endif
        list++;
    }

    return 1;
}

extern "C" void get_image_set_display_type(int type)
{
#if DEBUG_IMGLIB
    printf("get_image_set_display_type(type=%d)\n", type);
#endif
    preferred_display_type = type;
}

extern "C" image *get_image(char *name, int preferred_width, int preferred_height, int flags)
{
#if DEBUG_IMGLIB
    printf(">>> get_image CALLED: name='%s', w=%d, h=%d, flags=%d\n",
           name, preferred_width, preferred_height, flags);
#endif

    // Check if already in registered lists (synthetic images)
    for (image_list *list = registered_lists; list; list = list->next) {
        if (list->name && strcmp(list->name, name) == 0) {
            // Found registered image - check if it's synthetic
            if (list->synth_func) {
                // Synthetic image - generate it
                image *img = list->synth_func(list, preferred_display_type,
                                             preferred_width, preferred_height);
                if (img) return img;
            }
        }
    }

    // Calculate preferred size for multi-size images
    int preferred_size = 0;

    // Special case: a-k images have only 2 sizes (base and .2)
    // Use a-k.2.png for cards taller than 200 pixels
    if (strcmp(name, "a-k") == 0) {
        if (preferred_height > 200) {
            preferred_size = 2;
        } else {
            preferred_size = 1;
        }
#if DEBUG_IMGLIB
        printf("get_image: a-k with preferred_width=%d > 200, using size 2\n", preferred_width);
#endif
    } else if (strcmp(name, "no-drop") == 0) {
        if (preferred_width > 50) {
            preferred_size = 50;
        } else {
            preferred_size = 25;
        }
    } else if (preferred_width > 0) {

        // For other multi-size images like suits
        // Available sizes: 9, 13, 26, 39
        const int available_sizes[] = {9, 13, 26, 39};
        int best_size = 0;
        int best_diff = 999999;

        for (int i = 0; i < 4; i++) {
            int diff = abs(preferred_width - available_sizes[i]);
            if (diff < best_diff) {
                best_diff = diff;
                best_size = available_sizes[i];
            }
        }
        preferred_size = best_size;

#if DEBUG_IMGLIB
        printf("get_image: preferred_width=%d, selected size=%d\n", preferred_width, preferred_size);
#endif
    }

    // Try to load from PNG file with size preference
    QPixmap *pixmap = load_png_file(name, preferred_size);
    if (!pixmap) {
        return nullptr;
    }

    // Scale if needed
    // CRITICAL: Never delete cached pixmap! Make a copy instead to avoid cache corruption
    if (flags & GI_NOT_BIGGER) {
        if (pixmap->width() > preferred_width || pixmap->height() > preferred_height) {
            QPixmap scaled = pixmap->scaled(preferred_width, preferred_height,
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
            // Don't delete the cached pixmap - create new scaled version instead
            pixmap = new QPixmap(scaled);
        } else {
            // Need to create a new pixmap even if not scaling, since cached pixmap
            // might be reused and we don't want multiple image* structs pointing to same QPixmap
            pixmap = new QPixmap(*pixmap);
        }
    } else {
        // Always create a copy to avoid multiple image* structs sharing the same cached QPixmap
        pixmap = new QPixmap(*pixmap);
    }

    // Create image_list structure for sprite sheets
    // These define how many sub-images are in the sprite
    image_list *list = (image_list *)malloc(sizeof(image_list));
    memset(list, 0, sizeof(image_list));
    list->name = strdup(name);

    // Set sprite sheet dimensions based on known images
    if (strcmp(name, "a-k") == 0 || strncmp(name, "a-k.", 4) == 0) {
        list->across = 2;  // 2 colors (red, black)
        list->down = 13;   // 13 values (A-K)
    } else if (strcmp(name, "suits") == 0 || strncmp(name, "suits.", 6) == 0) {
        list->across = 1;  // 1 column
        list->down = 4;    // 4 suits (clubs, diamonds, spades, hearts)
    } else if (strcmp(name, "jack") == 0 || strcmp(name, "queen") == 0 || strcmp(name, "king") == 0) {
        list->across = 1;
        list->down = 1;
    } else if (strcmp(name, "penguin") == 0 || strcmp(name, "back-tile") == 0) {
        list->across = 1;
        list->down = 1;
    } else {
        // Default for unknown images
        list->across = 1;
        list->down = 1;
    }

    // Create image structure
    image *img = (image *)malloc(sizeof(image));
    memset(img, 0, sizeof(image));
    img->width = pixmap->width();
    img->height = pixmap->height();
    img->pixels = create_qt_pixels(pixmap);
    img->type = TABLE_COLOR; // All Qt images are color
    img->file_data = nullptr;
    img->next = nullptr;
    img->list = list;  // Point to the sprite sheet info
    img->synth_func = nullptr;

    return img;
}

extern "C" image *scale_image(image *src, int new_width, int new_height)
{
    if (!src || !src->pixels) {
        return nullptr;
    }

    // Get source pixmap
    qt_pixels_type *src_qp = (qt_pixels_type *)src->pixels;
    if (!src_qp || !src_qp->pixmap) {
        return nullptr;
    }

    // Scale the pixmap
    QPixmap scaled = src_qp->pixmap->scaled(new_width, new_height,
                                            Qt::IgnoreAspectRatio,
                                            Qt::SmoothTransformation);

    // Create new image structure
    image *new_img = (image *)malloc(sizeof(image));
    memset(new_img, 0, sizeof(image));
    new_img->width = scaled.width();
    new_img->height = scaled.height();

    // Create Qt pixels structure
    qt_pixels_type *new_qp = (qt_pixels_type *)malloc(sizeof(qt_pixels_type));
    memset(new_qp, 0, sizeof(qt_pixels_type));
    new_qp->pixmap = new QPixmap(scaled);

    new_img->pixels = (void *)new_qp;
    new_img->list = src->list;  // Keep reference to original list
    new_img->next = nullptr;
    new_img->synth_func = nullptr;

    return new_img;
}

extern "C" void put_image(image *src, int x, int y, int w, int h,
                          image *dest, int dx, int dy, int flags)
{
    if (!src || !src->pixels) {
#if DEBUG_IMGLIB
        printf("put_image: src is null\n");
#endif
        return;
    }

    // If image needs synthesis, call synth_func first
    if (src->synth_func) {
        src->synth_func(src);
    }

    qt_pixels_type *src_qp = (qt_pixels_type *)src->pixels;
    QPixmap *src_pixmap = src_qp->pixmap;

    if (!src_pixmap || src_pixmap->isNull()) {
#if DEBUG_IMGLIB
        printf("put_image: src pixmap is null or invalid\n");
#endif
        return;
    }

    // Determine destination
    QPixmap *dest_pixmap = nullptr;
    extern QPixmap *qt_backbuffer; // From qwin.cpp
    extern QWidget *qt_window; // From qwin.cpp

    if (dest && dest->pixels) {
        // Drawing to a specific image (e.g., during card synthesis)
        qt_pixels_type *dest_qp = (qt_pixels_type *)dest->pixels;
        dest_pixmap = dest_qp->pixmap;

        if (!dest_pixmap) {
#if DEBUG_IMGLIB
            printf("put_image: dest pixmap is null\n");
#endif
            return;
        }
    } else {
        // Draw to display (backbuffer)
        dest_pixmap = qt_backbuffer;

        if (!dest_pixmap) {
#if DEBUG_IMGLIB
            printf("put_image: no destination pixmap\n");
#endif
            return;
        }
    }

    // Extract the specified rectangle from source FIRST (before rotation)
    // X11 draws to destination at (dx+x, dy+y), not (dx, dy)
    // This is critical for taipei tile synthesis with negative offsets
    QRect src_rect(x, y, w, h);
    QRect dest_rect(dx + x, dy + y, w, h);

    // For rotation/inversion, check cache first or create and cache the transformation
    QPixmap draw_pixmap;
    if (flags & (PUT_ROTATED | PUT_INVERTED)) {
        // PERFORMANCE: Check if we have a cached transformed version
        bool use_cache = (x == 0 && y == 0 && w == src->width && h == src->height);

        if (use_cache && (flags & PUT_ROTATED) && !(flags & PUT_INVERTED)) {
            // Check rotated cache
            if (!src_qp->rotated) {
                // Create and cache rotated version
                QTransform transform;
                transform.rotate(180);  // Rotate 180 degrees (upside down) for playing cards
                src_qp->rotated = new QPixmap(src_pixmap->transformed(transform));
#if DEBUG_IMGLIB
                printf("put_image: cached rotated version of card\n");
#endif
            }
            draw_pixmap = *src_qp->rotated;
            src_rect = QRect(0, 0, draw_pixmap.width(), draw_pixmap.height());
        } else if (use_cache && (flags & PUT_INVERTED) && !(flags & PUT_ROTATED)) {
            // Check inverted cache
            if (!src_qp->inverted) {
                // Create and cache inverted version
                QImage inverted = src_pixmap->toImage();
                inverted.invertPixels();
                src_qp->inverted = new QPixmap(QPixmap::fromImage(inverted));
#if DEBUG_IMGLIB
                printf("put_image: cached inverted version of card\n");
#endif
            }
            draw_pixmap = *src_qp->inverted;
            src_rect = QRect(0, 0, draw_pixmap.width(), draw_pixmap.height());
        } else {
            // Sub-rectangle or combined flags - can't cache, do it on the fly
            draw_pixmap = src_pixmap->copy(src_rect);

            // Apply rotation if needed
            if (flags & PUT_ROTATED) {
                QTransform transform;
                transform.rotate(180);
                draw_pixmap = draw_pixmap.transformed(transform);
            }

            // Apply inversion if needed
            if (flags & PUT_INVERTED) {
                QImage inverted = draw_pixmap.toImage();
                inverted.invertPixels();
                draw_pixmap = QPixmap::fromImage(inverted);
            }

            src_rect = QRect(0, 0, draw_pixmap.width(), draw_pixmap.height());
        }
    }

    // PERFORMANCE: Batch painter optimization for backbuffer only
    // CRITICAL: Use local painter for card/tile synthesis to avoid nested painter conflicts
    QPainter *painter = nullptr;
    QPainter local_painter;
    bool use_local = (dest != nullptr);  // If dest is set, we're drawing to card/tile (not backbuffer)

    if (use_local) {
        // Drawing to card/tile during synthesis - use local painter to avoid conflicts
        if (!local_painter.begin(dest_pixmap)) {
#if DEBUG_IMGLIB
            printf("put_image: failed to begin local painter for synthesis\n");
#endif
            return;
        }
        painter = &local_painter;
#if DEBUG_IMGLIB
        printf("put_image: using LOCAL painter for synthesis (dest=%p)\n", dest);
#endif
    } else {
        // Drawing to backbuffer - use batch painter for performance
        painter = get_batch_painter(dest_pixmap);
        if (!painter) {
#if DEBUG_IMGLIB
            printf("put_image: failed to get batch painter\n");
#endif
            return;
        }

        // Apply clip region if enabled
        if (clip_enabled) {
            painter->setClipRect(clip_rect);
        } else {
            painter->setClipping(false);
        }
#if DEBUG_IMGLIB
        printf("put_image: using BATCH painter for backbuffer\n");
#endif
    }

    // Verify painter is active before drawing
    if (!painter->isActive()) {
#if DEBUG_IMGLIB
        printf("put_image: painter is not active\n");
#endif
        if (use_local) {
            local_painter.end();
        }
        return;
    }

    // Verify pixmaps are valid before drawing
    if (flags & (PUT_ROTATED | PUT_INVERTED)) {
        if (draw_pixmap.isNull()) {
#if DEBUG_IMGLIB
            printf("put_image: draw_pixmap is null\n");
#endif
            if (use_local) local_painter.end();
            return;
        }
        painter->drawPixmap(dest_rect, draw_pixmap, src_rect);
    } else {
        if (!src_pixmap || src_pixmap->isNull()) {
#if DEBUG_IMGLIB
            printf("put_image: src_pixmap is null before drawing\n");
#endif
            if (use_local) local_painter.end();
            return;
        }
        painter->drawPixmap(dest_rect, *src_pixmap, src_rect);
    }

    // End local painter if used
    if (use_local) {
        local_painter.end();
    }

    // Trigger window repaint when drawing to backbuffer
    // Note: update() is async and Qt will coalesce multiple calls
    if (!dest && qt_window) {
        qt_window->update();
    }

#if DEBUG_IMGLIB
    static int draw_count = 0;
    if (draw_count++ < 5) {
        printf("put_image: drew from (%d,%d,%d,%d) to (%d,%d) flags=%d\n",
               x, y, w, h, dx, dy, flags);
    }
#endif
}

extern "C" void put_mask(image *src, int x, int y, int w, int h,
                         image *dest, int dx, int dy, int flags)
{
#if DEBUG_IMGLIB
    printf("put_mask: applying alpha mask from source to dest\n");
#endif

    if (!src || !src->pixels || !dest || !dest->pixels) {
        return;
    }

    qt_pixels_type *src_qp = (qt_pixels_type *)src->pixels;
    qt_pixels_type *dest_qp = (qt_pixels_type *)dest->pixels;

    QPixmap *src_pixmap = src_qp->pixmap;
    QPixmap *dest_pixmap = dest_qp->pixmap;

    if (!src_pixmap || !dest_pixmap) {
        return;
    }

    // Extract the mask region from source
    QPixmap mask_region = src_pixmap->copy(x, y, w, h);

    // Use the alpha channel of the source as a mask to cut out transparency in dest
    // Convert to QImage to manipulate alpha
    QImage dest_img = dest_pixmap->toImage();
    QImage mask_img = mask_region.toImage();

    // Calculate destination position: dx+x, dy+y (like X11 XCopyArea)
    int dest_x = dx + x;
    int dest_y = dy + y;

    // Apply mask: copy alpha channel from source to dest at (dest_x, dest_y)
    for (int py = 0; py < mask_img.height(); py++) {
        for (int px = 0; px < mask_img.width(); px++) {
            int target_x = px + dest_x;
            int target_y = py + dest_y;

            // Bounds check
            if (target_x < 0 || target_x >= dest_img.width() ||
                target_y < 0 || target_y >= dest_img.height()) {
                continue;
            }

            QRgb mask_pixel = mask_img.pixel(px, py);
            int src_alpha = qAlpha(mask_pixel);

            if (src_alpha < 255) {
                // Make destination pixel more transparent based on source alpha
                QRgb dest_pixel = dest_img.pixel(target_x, target_y);
                int dest_alpha = qAlpha(dest_pixel);
                // Multiply alpha values (if source is transparent, make dest transparent)
                int new_alpha = (dest_alpha * src_alpha) / 255;
                dest_img.setPixel(target_x, target_y, qRgba(qRed(dest_pixel), qGreen(dest_pixel),
                                                             qBlue(dest_pixel), new_alpha));
            }
        }
    }

    // Convert back to pixmap
    *dest_pixmap = QPixmap::fromImage(dest_img);
}

extern "C" void put_subimage(image *src, int col, int row,
                             image *dest, int dx, int dy, int flags)
{
#if DEBUG_IMGLIB
    printf("put_subimage(col=%d, row=%d, dx=%d, dy=%d)\n", col, row, dx, dy);
#endif

    if (!src || !src->list) {
        put_image(src, 0, 0, src->width, src->height, dest, dx, dy, flags);
        return;
    }

    // Calculate subimage position
    int across = src->list->across;
    int down = src->list->down;

    if (across == 1 && down == 1) {
        col = row = 0;
    }

    int w = src->width / across;
    int h = src->height / down;
    int x = col * w;
    int y = row * h;

    // Compensate for put_image's (dx+x, dy+y) behavior
    // We want final position at (dx, dy), so pass (dx-x, dy-y)
    put_image(src, x, y, w, h, dest, dx - x, dy - y, flags);
}

extern "C" void fill_image(image *dest, int x, int y, int w, int h,
                           int r, int g, int b)
{
    // Determine destination
    QPixmap *dest_pixmap = nullptr;
    extern QPixmap *qt_backbuffer;

    if (dest && dest->pixels) {
        // Drawing to a specific image (e.g., during card synthesis)
        qt_pixels_type *dest_qp = (qt_pixels_type *)dest->pixels;
        dest_pixmap = dest_qp->pixmap;
    } else {
        // Drawing to display (backbuffer)
        dest_pixmap = qt_backbuffer;
    }

    if (!dest_pixmap) return;

    // PERFORMANCE: Batch painter optimization for backbuffer only
    // CRITICAL: Use local painter for card/tile synthesis to avoid nested painter conflicts
    QPainter *painter = nullptr;
    QPainter local_painter;
    bool use_local = (dest != nullptr);  // If dest is set, we're drawing to card/tile (not backbuffer)

    if (use_local) {
        // Drawing to card/tile during synthesis - use local painter to avoid conflicts
        if (!local_painter.begin(dest_pixmap)) {
#if DEBUG_IMGLIB
            printf("fill_image: failed to begin local painter for synthesis\n");
#endif
            return;
        }
        painter = &local_painter;
#if DEBUG_IMGLIB
        printf("fill_image: using LOCAL painter for synthesis (dest=%p)\n", dest);
#endif
    } else {
        // Drawing to backbuffer - use batch painter for performance
        painter = get_batch_painter(dest_pixmap);
        if (!painter) return;

        // Apply clip region if enabled
        if (clip_enabled) {
            painter->setClipRect(clip_rect);
        } else {
            painter->setClipping(false);
        }
#if DEBUG_IMGLIB
        printf("fill_image: using BATCH painter for backbuffer\n");
#endif
    }

    painter->fillRect(x, y, w, h, QColor(r, g, b));

    // End local painter if used
    if (use_local) {
        local_painter.end();
    }
}

extern "C" image *alloc_synth_image(image_list *list, int width, int height, int type)
{
#if DEBUG_IMGLIB
    printf("alloc_synth_image(w=%d, h=%d, type=%d)\n", width, height, type);
#endif

    // Create a blank QPixmap
    QPixmap *pixmap = new QPixmap(width, height);
    pixmap->fill(Qt::transparent);

    // Create image structure
    image *img = (image *)malloc(sizeof(image));
    memset(img, 0, sizeof(image));
    img->width = width;
    img->height = height;
    img->pixels = create_qt_pixels(pixmap);
    img->type = type;
    img->list = list;
    img->synth_func = nullptr;

    // Add to cache linked list
    img->next = list->subimage[type];
    list->subimage[type] = img;

    return img;
}
