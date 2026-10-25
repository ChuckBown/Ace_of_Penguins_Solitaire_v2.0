/* The Ace of Penguins - qwin.cpp
   Qt5 implementation of xwin.c X11 functions
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

#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <QPixmap>
#include <QIcon>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QPaintEvent>
#include <QCloseEvent>
#include <QTimer>
#include <QFont>
#include <QFontMetrics>
#include <QDir>
#include <QSurfaceFormat>
#include <QScreen>
#include <QGuiApplication>
#include <cstdio>
#include <cstring>

extern "C" {
#include "imagelib.h"
#include "xwin.h"
#include "cards.h"
}

// From qimagelib.cpp - batch painter functions
class QPainter;
class QPixmap;
extern QPainter* get_batch_painter(QPixmap *target_pixmap);
extern "C" void end_paint_batch();

// Debug mode - prints stub calls
#define DEBUG_QWIN 0

// Forward declaration
class TableWidget;
class HelpWidget;

// Forward declare the actual function from qrc_common.cpp
extern int qInitResources_common();
extern int qCleanupResources_common();

// Extern declaration for help widget from qhelp.cpp
extern HelpWidget *global_help_widget;

// C++ helper function to initialize resources (not extern "C")
static void initQtResources() {

    // Call the resource init function directly
    int result = qInitResources_common();

#if DEBUG_QWIN
    printf("DEBUG: qInitResources_common() returned: %d\n", result);

    // Verify resources are accessible
    QFile test(":/images/png/a-k.1.png");
    printf("DEBUG: Resource :/images/png/a-k.1.png exists: %s\n", test.exists() ? "YES" : "NO");

    // Try listing all resources
    QDir resDir(":/images/game");
    printf("DEBUG: :/images/game directory exists: %s\n", resDir.exists() ? "YES" : "NO");
    if (resDir.exists()) {
        QStringList files = resDir.entryList();
        printf("DEBUG: :/images/game contains %d files:\n", files.size());
        for (const QString &file : files) {
            printf("  - %s\n", file.toUtf8().constData());
        }
    }

    // Try listing all resources
    QDir resDir2(":/images/png");
    printf("DEBUG: :/images/png directory exists: %s\n", resDir2.exists() ? "YES" : "NO");
    if (resDir2.exists()) {
        QStringList files = resDir2.entryList();
        printf("DEBUG: :/images/png contains %d files:\n", files.size());
        for (const QString &file : files) {
            printf("  - %s\n", file.toUtf8().constData());
        }
    }
#endif
}

// Global Qt objects
static QApplication *qt_app = nullptr;
static QPainter *qt_painter = nullptr;

// Shared with qimagelib.cpp for rendering
TableWidget *qt_window = nullptr;
QPixmap *qt_backbuffer = nullptr;

// Display dimensions
int display_width = 800;
int display_height = 600;
int table_width = 640;
int table_height = 480;

// PERFORMANCE: Cache common colors to avoid repeated QColor construction
static const QColor table_green(0x00, 0x66, 0x00);   // RGB(0, 102, 0)
static const QColor white_color(255, 255, 255);

// Qt Pixmap type for images
typedef struct {
    QPixmap *image_pixmap;
    QPixmap *image_mask;
    QPixmap *rotated_pixmap;
    QPixmap *rotated_mask;
    QPixmap *inverted_pixmap;
} qt_pixels_type;

// Event queue for xwin_nextevent
#include <queue>
static std::queue<XWin_Event> event_queue;

// ============================================================================
// CUSTOM WIDGET CLASS - Handles Qt events and painting
// ============================================================================

class TableWidget : public QWidget {
    // Note: No Q_OBJECT macro - we use lambda for timer callback instead of slots

public:
    // PERFORMANCE: Public accessors for drag state
    void clearDragEventPending() { drag_event_pending = false; }
    bool isDragging() const { return is_dragging; }

    TableWidget(QWidget *parent = nullptr) : QWidget(parent),
        resize_timer(nullptr), pending_width(0), pending_height(0),
        is_dragging(false), drag_event_pending(false) {
        setMouseTracking(true);  // Receive mouse motion even without buttons pressed
        setFocusPolicy(Qt::StrongFocus);  // Accept keyboard focus
        setAttribute(Qt::WA_OpaquePaintEvent);  // We paint entire widget, no need to preserve old content
        setAttribute(Qt::WA_NoSystemBackground);  // Don't let system fill background
        setAutoFillBackground(false);  // We handle all background painting
        setAttribute(Qt::WA_InputMethodEnabled);  // Enable virtual keyboard / input method support (Steam Deck OSK)

        // PERFORMANCE: Create debounce timer for resize events
        // This prevents expensive card re-synthesis during window dragging
        resize_timer = new QTimer(this);
        resize_timer->setSingleShot(true);  // Only fire once after delay
        resize_timer->setInterval(200);  // 200ms delay after resize stops

        // Connect timer to lambda that processes the resize
        connect(resize_timer, &QTimer::timeout, [this]() {
            processResize();
        });
    }

private:
    // PERFORMANCE: Process resize after debounce delay
    // This method is called 200ms after the last resize event
    void processResize() {
#if DEBUG_QWIN
        printf("RESIZE DEBOUNCE: Processing queued resize to %d x %d\n", pending_width, pending_height);
#endif

        // Queue resize event to game for expensive redraw
        XWin_Event ev;
        ev.type = ev_resize;
        ev.x = 0;
        ev.y = 0;
        ev.w = pending_width;
        ev.h = pending_height;
        event_queue.push(ev);
    }

    QTimer *resize_timer;
    int pending_width;
    int pending_height;

    // PERFORMANCE: Track drag state for event compression
    bool is_dragging;
    bool drag_event_pending;

protected:
    // Paint the backbuffer to the window
    void paintEvent(QPaintEvent *event) override {
        Q_UNUSED(event);

        QPainter painter(this);
#if DEBUG_QWIN
        printf("PAINT: widget %dx%d, backbuffer=%p", width(), height(), qt_backbuffer);
        if (qt_backbuffer) {
            printf(" %dx%d\n", qt_backbuffer->width(), qt_backbuffer->height());
        } else {
            printf(" (null)\n");
        }
#endif
        // PERFORMANCE: Draw backbuffer directly - it already has green background
        // Only fill widget background if backbuffer doesn't exist (shouldn't happen)
        if (qt_backbuffer) {
            painter.drawPixmap(0, 0, *qt_backbuffer);
        } else {
            // Fallback: fill with green if backbuffer is missing
            painter.fillRect(0, 0, width(), height(), table_green);
        }

        // DON'T queue expose events here - that creates an infinite loop!
        // Expose events should only be queued on initial show and resize
    }

    // Mouse button press
    void mousePressEvent(QMouseEvent *event) override {
        // PERFORMANCE: Track that we're starting a drag
        is_dragging = true;
        drag_event_pending = false;

        XWin_Event ev;
        ev.type = ev_buttondown;
        ev.x = event->pos().x();
        ev.y = event->pos().y();
        ev.button = event->button() == Qt::LeftButton ? 1 :
                    event->button() == Qt::MiddleButton ? 2 :
                    event->button() == Qt::RightButton ? 3 :
                    event->button() == Qt::XButton1 ? 4 :
                    event->button() == Qt::XButton2 ? 5 : 0;
        ev.shifts = 0;
        if (event->modifiers() & Qt::ShiftModifier) ev.shifts |= ES_SHIFT;
        if (event->modifiers() & Qt::ControlModifier) ev.shifts |= ES_CONTROL;
        if (event->modifiers() & Qt::AltModifier) ev.shifts |= ES_ALT;
        event_queue.push(ev);

#if DEBUG_QWIN
        printf("Mouse press at (%d, %d) button %d\n", ev.x, ev.y, ev.button);
#endif
    }

    // Mouse button release
    void mouseReleaseEvent(QMouseEvent *event) override {
        // PERFORMANCE: End drag state
        is_dragging = false;
        drag_event_pending = false;

        XWin_Event ev;
        ev.type = ev_buttonup;
        ev.x = event->pos().x();
        ev.y = event->pos().y();
        ev.button = event->button() == Qt::LeftButton ? 1 :
                    event->button() == Qt::MiddleButton ? 2 :
                    event->button() == Qt::RightButton ? 3 :
                    event->button() == Qt::XButton1 ? 4 :
                    event->button() == Qt::XButton2 ? 5 : 0;
        ev.shifts = 0;
        if (event->modifiers() & Qt::ShiftModifier) ev.shifts |= ES_SHIFT;
        if (event->modifiers() & Qt::ControlModifier) ev.shifts |= ES_CONTROL;
        if (event->modifiers() & Qt::AltModifier) ev.shifts |= ES_ALT;
        event_queue.push(ev);

#if DEBUG_QWIN
        printf("Mouse release at (%d, %d) button %d\n", ev.x, ev.y, ev.button);
#endif
    }

    // Mouse double-click
    void mouseDoubleClickEvent(QMouseEvent *event) override {
        XWin_Event ev;
        ev.type = ev_doubleclick;
        ev.x = event->pos().x();
        ev.y = event->pos().y();
        ev.button = event->button() == Qt::LeftButton ? 1 :
                    event->button() == Qt::MiddleButton ? 2 :
                    event->button() == Qt::RightButton ? 3 :
                    event->button() == Qt::XButton1 ? 4 :
                    event->button() == Qt::XButton2 ? 5 : 0;
        ev.shifts = 0;
        if (event->modifiers() & Qt::ShiftModifier) ev.shifts |= ES_SHIFT;
        if (event->modifiers() & Qt::ControlModifier) ev.shifts |= ES_CONTROL;
        if (event->modifiers() & Qt::AltModifier) ev.shifts |= ES_ALT;
        event_queue.push(ev);

#if DEBUG_QWIN
        printf("Mouse double-click at (%d, %d) button %d\n", ev.x, ev.y, ev.button);
#endif
    }

    // Mouse motion
    void mouseMoveEvent(QMouseEvent *event) override {
        // PERFORMANCE (Option 1): Event compression during drag
        // If we're dragging and already have a pending drag event, skip this one
        // The game will process the most recent position when it's ready
        if (is_dragging && drag_event_pending) {
            // Already have a drag event queued - skip intermediate moves
            return;
        }

        XWin_Event ev;
        ev.type = ev_motion;
        ev.x = event->pos().x();
        ev.y = event->pos().y();
        ev.button = 0;
        ev.shifts = 0;
        if (event->buttons() & Qt::LeftButton) ev.shifts |= 0x0100;  // Button1Mask
        if (event->buttons() & Qt::MiddleButton) ev.shifts |= 0x0200;  // Button2Mask
        if (event->buttons() & Qt::RightButton) ev.shifts |= 0x0400;  // Button3Mask
        if (event->modifiers() & Qt::ShiftModifier) ev.shifts |= ES_SHIFT;
        if (event->modifiers() & Qt::ControlModifier) ev.shifts |= ES_CONTROL;
        if (event->modifiers() & Qt::AltModifier) ev.shifts |= ES_ALT;
        event_queue.push(ev);

        // Mark that we have a pending drag event
        if (is_dragging) {
            drag_event_pending = true;
        }
    }

    // Key press
    void keyPressEvent(QKeyEvent *event) override {
        XWin_Event ev;
        ev.type = ev_keypress;
        ev.x = 0;
        ev.y = 0;
        ev.button = 0;
        ev.shifts = 0;
        if (event->modifiers() & Qt::ShiftModifier) ev.shifts |= ES_SHIFT;
        if (event->modifiers() & Qt::ControlModifier) ev.shifts |= ES_CONTROL;
        if (event->modifiers() & Qt::AltModifier) ev.shifts |= ES_ALT;

        // Convert Qt key to character
        QString text = event->text();
        if (!text.isEmpty()) {
            ev.key = text[0].toLatin1();
        } else {
            // Handle special keys
            switch (event->key()) {
                case Qt::Key_Escape: ev.key = 27; break;
                case Qt::Key_Return:
                case Qt::Key_Enter: ev.key = 13; break;
                case Qt::Key_Backspace: ev.key = 8; break;
                case Qt::Key_Tab: ev.key = 9; break;
                case Qt::Key_Space: ev.key = ' '; break;
                // Function keys: KEY_F(x) = 0x100 + x
                case Qt::Key_F1: ev.key = 0x101; break;
                case Qt::Key_F2: ev.key = 0x102; break;
                case Qt::Key_F3: ev.key = 0x103; break;
                case Qt::Key_F4: ev.key = 0x104; break;
                case Qt::Key_F5: ev.key = 0x105; break;
                case Qt::Key_F6: ev.key = 0x106; break;
                case Qt::Key_F7: ev.key = 0x107; break;
                case Qt::Key_F8: ev.key = 0x108; break;
                case Qt::Key_F9: ev.key = 0x109; break;
                case Qt::Key_F10: ev.key = 0x10A; break;
                case Qt::Key_F11: ev.key = 0x10B; break;
                case Qt::Key_F12: ev.key = 0x10C; break;
                default: ev.key = 0; break;
            }
        }

        if (ev.key) {
            event_queue.push(ev);
#if DEBUG_QWIN
            printf("Key press: %c (0x%02x)\n", ev.key, ev.key);
#endif
        }
    }

    // Input method event - for virtual keyboards (Steam Deck OSK)
    void inputMethodEvent(QInputMethodEvent *event) override {
        QString text = event->commitString();
        if (!text.isEmpty()) {
#if DEBUG_QWIN
            printf("Input method commit: '%s'\n", text.toUtf8().constData());
#endif
            // Convert each character in the committed text to a key event
            for (int i = 0; i < text.length(); ++i) {
                QChar ch = text[i];

                // Create a QKeyEvent and process it through keyPressEvent
                Qt::KeyboardModifiers modifiers = Qt::NoModifier;
                int key = ch.unicode();

                // Map common characters to Qt key codes (Qt6-compatible comparison)
                if (ch == QLatin1Char('\n') || ch == QLatin1Char('\r')) {
                    key = Qt::Key_Return;
                } else if (ch == QLatin1Char('\t')) {
                    key = Qt::Key_Tab;
                } else if (ch == QLatin1Char(' ')) {
                    key = Qt::Key_Space;
                } else if (ch.unicode() == 0x1B) {  // ESC
                    key = Qt::Key_Escape;
                } else if (ch.unicode() == 0x08) {  // Backspace
                    key = Qt::Key_Backspace;
                }

                QKeyEvent keyEvent(QKeyEvent::KeyPress, key, modifiers, QString(ch));
                keyPressEvent(&keyEvent);
            }
        }

        QWidget::inputMethodEvent(event);
    }

    // Window resize
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);

        QPixmap *old_backbuffer = qt_backbuffer;

        // CRITICAL: End any active paint batch before destroying backbuffer
        // Otherwise QPainter will be painting to a destroyed QPixmap
        end_paint_batch();

        // Recreate backbuffer at new size
        if (qt_backbuffer) {
            delete qt_backbuffer;
            qt_backbuffer = nullptr;
        }

#if DEBUG_QWIN
        printf("RESIZE: deleted old backbuffer %p, creating new one at %dx%d\n",
               old_backbuffer, width(), height());
#endif

        qt_backbuffer = new QPixmap(width(), height());
        qt_backbuffer->fill(table_green);

#if DEBUG_QWIN
        printf("RESIZE: new backbuffer created at %p, filled with green\n", qt_backbuffer);
#endif

        // Update globals
        table_width = width();
        table_height = height();

        // Resize help overlay if visible (cast to QWidget* since HelpWidget is only forward-declared)
        if (global_help_widget) {
            QWidget *help_as_widget = reinterpret_cast<QWidget*>(global_help_widget);
            if (help_as_widget->isVisible()) {
                help_as_widget->setGeometry(rect());
            }
        }

        // Force SYNCHRONOUS repaint to immediately show clean backbuffer
        repaint();

        // PERFORMANCE: Debounce resize events to prevent expensive redraws during drag
        // Store pending size and restart timer - only process after 200ms of no resizing
        pending_width = width();
        pending_height = height();
        resize_timer->start();  // Restart timer (debounce)

#if DEBUG_QWIN
        printf("RESIZE: Debounced - will process resize to %d x %d after 200ms\n", width(), height());
#endif
    }

    // Window close
    void closeEvent(QCloseEvent *event) override {
        Q_UNUSED(event);

        // Queue quit event
        XWin_Event ev;
        ev.type = ev_quit;
        ev.x = 0;
        ev.y = 0;
        event_queue.push(ev);

#if DEBUG_QWIN
        printf("Window close requested\n");
#endif
    }
};

// ============================================================================
// PUBLIC API FUNCTIONS (extern "C")
// ============================================================================

// Static storage for program name (for window title) and argc/argv
// Qt's QApplication stores references to argc/argv, so they must persist
static QString program_name;
static int saved_argc = 0;
static char **saved_argv = nullptr;

extern "C" const char *xwin_get_argv0()
{
    return saved_argv ? saved_argv[0] : nullptr;
}

extern "C" int xwin_init(int argc, char **argv)
{
#if DEBUG_QWIN
    printf("STUB: xwin_init(argc=%d)\n", argc);
#endif

    // TODO: Parse command line options (-rotate, -visual, etc.)
    // TODO: Detect actual display dimensions
    // TODO: Set up color handling (mono/gray/color detection)

    // Save argc/argv for QApplication (it stores references, not copies)
    saved_argc = argc;
    saved_argv = argv;

    // Capture program name from argv[0] for window title
    if (argc > 0 && argv[0]) {
        // Extract just the executable name without path
        QString full_path = QString::fromUtf8(argv[0]);
        int slash_pos = full_path.lastIndexOf('/');
        if (slash_pos == -1) {
            slash_pos = full_path.lastIndexOf('\\'); // Windows path
        }
        program_name = full_path.mid(slash_pos + 1);

        // Remove _qt suffix and .exe extension
        program_name.replace("_qt", "");
        program_name.replace(".exe", "", Qt::CaseInsensitive);

        // Capitalize first letter
        if (program_name.length() > 0) {
            program_name[0] = program_name[0].toUpper();
        }
    }

    // Check if QApplication already exists (e.g., created by launcher)
    QApplication *existing_app = qobject_cast<QApplication*>(QApplication::instance());

    if (!existing_app) {
        // No QApplication exists yet, create one

        // PERFORMANCE: Enable OpenGL-accelerated rendering for better performance on Windows
        // This must be set BEFORE QApplication is created
#ifdef _WIN32
        // Use desktop OpenGL on Windows (more compatible than ANGLE)
        QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
#endif

        // PERFORMANCE: Configure OpenGL surface format for hardware acceleration
        // This applies to all widgets and provides anti-aliasing
        QSurfaceFormat format;
        format.setSamples(4); // 4x multi-sampling anti-aliasing for smoother graphics
        QSurfaceFormat::setDefaultFormat(format);

        // Suppress common Qt platform warnings that don't affect functionality
        // (XInput2, XKeyboard, OpenGL warnings on minimal X servers)
        qputenv("QT_LOGGING_RULES", "qt.qpa.xcb=false;qt.xkb.compose=false");

        // Use saved static argc/argv so they remain valid
        qt_app = new QApplication(saved_argc, saved_argv);
    } else {
        // Use the existing QApplication instance
        qt_app = existing_app;
#if DEBUG_QWIN
        printf("  Using existing QApplication instance at %p\n", qt_app);
#endif
    }

    // Initialize embedded resources (safe to call multiple times)
    // Qt resources in static libraries must be explicitly initialized
    // This registers the common.qrc resources (cards, suits, shared images)
    initQtResources();

    // For now, use default desktop dimensions
    display_width = 1024;
    display_height = 768;

    return 0; // Success
}

extern "C" void xwin_create(int width, int height)
{
#if DEBUG_QWIN
    printf("xwin_create(width=%d, height=%d) - window=%p\n", width, height, qt_window);
#endif

    // If window already exists, just resize it
    if (qt_window) {
#if DEBUG_QWIN
        printf("  Window already exists, resizing to %dx%d\n", width, height);
#endif
        qt_window->resize(width, height);
        table_width = width;
        table_height = height;

        // CRITICAL: End any active paint batch before destroying backbuffer
        end_paint_batch();

        // Recreate backbuffer at new size
        if (qt_backbuffer) {
            delete qt_backbuffer;
        }
        qt_backbuffer = new QPixmap(width, height);
        qt_backbuffer->fill(table_green);
        return;
    }

    // Create window for the first time
    qt_window = new TableWidget();

    // Set window title: "Ace of Penguins - GameName"
    QString window_title = "Ace of Penguins";
    if (!program_name.isEmpty()) {
        window_title += " - " + program_name;
    }
    qt_window->setWindowTitle(window_title);

    // Set window icon - try synthesized "as" card first, fallback to penguin
    QIcon window_icon;
    bool icon_set = false;

    // Try to use synthesized ace of spades card first
    Picture *as_card = get_picture((char*)"as");
    if (as_card && as_card->w > 0) {
        image *as_img = reinterpret_cast<image*>(as_card);
        if (as_img && as_img->pixels) {
            qt_pixels_type *as_qp = reinterpret_cast<qt_pixels_type*>(as_img->pixels);
            if (as_qp && as_qp->image_pixmap) {
                window_icon = QIcon(*as_qp->image_pixmap);
                icon_set = true;
#if DEBUG_QWIN
                printf("  Set window icon from synthesized 'as' card (%dx%d)\n",
                       as_qp->image_pixmap->width(), as_qp->image_pixmap->height());
#endif
            }
        }
    }

    // Fallback to penguin if card synthesis failed
    if (!icon_set) {
        QPixmap icon_pixmap(":/images/png/big-penguin.png");
        if (!icon_pixmap.isNull()) {
            window_icon = QIcon(icon_pixmap);
            icon_set = true;
#if DEBUG_QWIN
            printf("  Set window icon from big-penguin.png (%dx%d)\n",
                   icon_pixmap.width(), icon_pixmap.height());
#endif
        }
    }

    // Only set icon if we successfully created one
    if (icon_set) {
        qt_window->setWindowIcon(window_icon);
    }

    qt_window->resize(width, height);

    table_width = width;
    table_height = height;

    // Create backbuffer for double-buffering (QPixmap uses hardware acceleration)
    qt_backbuffer = new QPixmap(width, height);
    qt_backbuffer->fill(table_green); // Table background RGB(0, 102, 0) from xwin.c

    // Call resize callback BEFORE showing window to pre-size cards while hidden
    // This prevents visible card re-synthesis after window appears
    extern void (*resize_cb)(int w, int h);
    if (resize_cb) {
#if DEBUG_QWIN
        printf("  Calling resize_cb(%d, %d) before show() to pre-size cards\n", width, height);
#endif
        resize_cb(width, height);
    }

    qt_window->show();

    // Queue initial expose event so game knows to draw
    XWin_Event ev;
    ev.type = ev_expose;
    ev.x = 0;
    ev.y = 0;
    ev.w = width;
    ev.h = height;
    event_queue.push(ev);
}

extern "C" int xwin_nextevent(XWin_Event *event)
{
    // Process Qt events first
    if (qt_app) {
        qt_app->processEvents();
    }

    // Check if we have queued events from TableWidget
    if (!event_queue.empty()) {
        *event = event_queue.front();
        event_queue.pop();

        // PERFORMANCE (Option 1): Clear drag_event_pending when motion event is consumed
        // This allows the next mouse move to be processed
        if (event->type == ev_motion && qt_window) {
            qt_window->clearDragEventPending();
        }

#if DEBUG_QWIN
        printf("xwin_nextevent: returning event type %d\n", event->type);
#endif
        return event->type;
    }

    // No events available
    event->type = ev_none;
    return ev_none;
}

extern "C" void xwin_fixed_size(int width, int height)
{
#if DEBUG_QWIN
    printf("STUB: xwin_fixed_size(width=%d, height=%d)\n", width, height);
#endif

    // TODO: Set minimum and maximum size hints
    // TODO: Disable resize decorations

    if (qt_window) {
        qt_window->setFixedSize(width, height);
    }
}

extern "C" void xwin_resize(int width, int height)
{
#if DEBUG_QWIN
    printf("xwin_resize(width=%d, height=%d)\n", width, height);
#endif

    if (qt_window) {
        // Update global table dimensions
        table_width = width;
        table_height = height;

        // End any active paint batch before destroying backbuffer
        end_paint_batch();

        // Recreate backbuffer at new size
        if (qt_backbuffer) {
            delete qt_backbuffer;
        }
        qt_backbuffer = new QPixmap(width, height);
        qt_backbuffer->fill(table_green);

        // Clear any fixed size constraint so the window can be resized
        qt_window->setMinimumSize(0, 0);
        qt_window->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

        // Resize the Qt window (this will trigger a resize event and repaint)
        qt_window->resize(width, height);
    }
}

extern "C" int xwin_check_size_fits(int width, int height)
{
#if DEBUG_QWIN
    printf("xwin_check_size_fits(width=%d, height=%d)\n", width, height);
#endif

    // Check if the requested size fits on the screen
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return 0;  // Can't determine, assume it doesn't fit
    }

    QRect available = screen->availableGeometry();

    // Check if requested size fits with some margin for window decorations
    const int decoration_margin = 50;  // Space for title bar, borders, taskbar
    if (width + decoration_margin > available.width() ||
        height + decoration_margin > available.height()) {
        return 0;  // Doesn't fit
    }

    return 1;  // Fits
}

// Global clip region state (shared with qimagelib.cpp)
QRect clip_rect;
bool clip_enabled = false;

extern "C" void xwin_clip(int x, int y, int w, int h)
{
#if DEBUG_QWIN
    printf("xwin_clip(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Store clip region - it will be applied in put_image
    clip_rect = QRect(x, y, w, h);
    clip_enabled = true;
}

extern "C" void xwin_noclip()
{
#if DEBUG_QWIN
    printf("xwin_noclip()\n");
#endif

    // Clear clip region
    clip_enabled = false;
}

extern "C" int pixel_for(int r, int g, int b)
{
#if DEBUG_QWIN
    static int call_count = 0;
    if (call_count++ < 5) {
        printf("STUB: pixel_for(r=%d, g=%d, b=%d)\n", r, g, b);
    }
#endif

    // TODO: Handle different visual types (mono/gray/color)
    // TODO: Apply gamma correction if needed
    // TODO: Handle color allocation/caching

    // For now, return QRgb value (Qt's pixel format)
    return qRgb(r, g, b);
}

// ============================================================================
// ADDITIONAL FUNCTIONS NEEDED BY table.c
// ============================================================================

extern "C" void flush()
{
#if DEBUG_QWIN
    printf("flush()\n");
#endif

    // PERFORMANCE: End painter batch before flushing to display
    end_paint_batch();

    // PERFORMANCE (Option 2): Lighter flush during dragging
    // Skip processEvents() during active drag to reduce event loop overhead
    // Just queue the update and let Qt batch it naturally
    if (qt_window) {
        qt_window->update();
    }

    // Only process events if not actively dragging
    // This prevents event loop churn during rapid mouse movement
    if (qt_app && qt_window && !qt_window->isDragging()) {
        qt_app->processEvents();
    }
}

extern "C" void flushsync()
{
#if DEBUG_QWIN
    printf("flushsync()\n");
#endif

    // PERFORMANCE: End painter batch before flushing to display
    end_paint_batch();

    // Synchronous flush - force immediate repaint
    if (qt_window) {
        qt_window->repaint();  // Synchronous repaint
    }
    if (qt_app) {
        qt_app->processEvents();
    }
}

extern "C" void beep()
{
#if DEBUG_QWIN
    printf("STUB: beep()\n");
#endif

    QApplication::beep();
}

extern "C" void clear(int x, int y, int w, int h)
{
#if DEBUG_QWIN
    printf("clear(x=%d, y=%d, w=%d, h=%d)\n", x, y, w, h);
#endif

    // Fill rectangle with background color
    if (qt_backbuffer) {
        // PERFORMANCE: Reuse batch painter to avoid repeated QPainter construction
        QPainter *painter = get_batch_painter(qt_backbuffer);
        if (painter) {
            // Apply clip region if enabled
            if (clip_enabled) {
                painter->setClipRect(clip_rect);
            } else {
                painter->setClipping(false);
            }

            painter->fillRect(x, y, w, h, table_green); // Table background RGB(0, 102, 0)
        }
    }

    // Trigger window update to show the cleared area
    if (qt_window) {
        qt_window->update();
    }
}

extern "C" void text(char *t, int x, int y)
{
#if DEBUG_QWIN
    printf("text(text='%s', x=%d, y=%d)\n", t, x, y);
#endif

    if (!qt_backbuffer || !t) {
        return;
    }

    // PERFORMANCE: Reuse batch painter to avoid repeated QPainter construction
    QPainter *painter = get_batch_painter(qt_backbuffer);
    if (!painter) return;

    // Use a reasonable fixed-width font
    QFont font("Courier", 12);
    painter->setFont(font);

    // Get font metrics to calculate text bounds
    QFontMetrics metrics(font);
    QString qtext = QString::fromUtf8(t);

    // Use horizontalAdvance (Qt5.11+) or width for actual text width including spaces
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
    int textWidth = metrics.horizontalAdvance(qtext);
#else
    int textWidth = metrics.width(qtext);
#endif

    // Clear the background area first (green table color)
    // Add some padding around the text
    int padding = 2;
    painter->fillRect(x - padding, y - metrics.ascent() - padding,
                    textWidth + padding * 2,
                    metrics.height() + padding * 2,
                    table_green);

    // Set white text color and draw
    painter->setPen(white_color);
    painter->drawText(x, y, qtext);

    // Trigger window update to show the text
    if (qt_window) {
        qt_window->update();
    }
}

// ============================================================================
// IMAGE FUNCTIONS - Implemented in qimagelib.cpp
// ============================================================================
// Note: put_image, put_mask, and fill_image are defined in qimagelib.cpp
