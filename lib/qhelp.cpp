/* The Ace of Penguins - qhelp.cpp
   Qt5 implementation of help.c help system
   Uses QTabWidget + QTextBrowser overlay 
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

#include <QWidget>
#include <QTabWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QKeyEvent>
#include <QFile>
#include <QTextStream>
#include <QString>
#include <QStringList>
#include <QPushButton>
#include <QRegularExpression>
#include <QBuffer>
#include <QByteArray>
#include <QPixmap>
#include <cstdio>

extern "C" {
#include "imagelib.h"
#include "cards.h"
#include "xwin.h"
#include "qscore.h"
}

// Debug mode
#define DEBUG_HELP 0

// Forward declarations from qwin.cpp
extern QWidget *qt_window;
extern int table_width, table_height;

// Save centered picture when showing help, restore when closing
static Picture *saved_centered_pic = nullptr;

// Help widget class - overlays the game window
class HelpWidget : public QWidget {
public:
    HelpWidget(QWidget *parent = nullptr) : QWidget(parent) {
        // Set up overlay properties
        setWindowFlags(Qt::Widget);  // Child widget, not a separate window
        setAttribute(Qt::WA_StyledBackground);

        // Create layout
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(10, 10, 10, 10);

        // Create tab widget for different help sections
        tab_widget = new QTabWidget(this);
        layout->addWidget(tab_widget);

        // Create close button
        QPushButton *close_btn = new QPushButton("Close (ESC)", this);
        connect(close_btn, &QPushButton::clicked, this, &HelpWidget::closeHelp);
        layout->addWidget(close_btn);

        // Set background color (dark green to match table)
        setStyleSheet("QWidget { background-color: rgb(0, 102, 0); }"
                     "QTabWidget::pane { background-color: rgb(0, 102, 0); }"
                     "QTabBar::tab { background-color: rgb(0, 80, 0); color: white; padding: 2px; "
                     "               border: 1px solid white; margin: 2px; }"
                     "QTabBar::tab:selected { background-color: rgb(0, 102, 0); }"
                     "QTextBrowser { background-color: rgb(0, 102, 0); color: white; }"
                     "QPushButton { background-color: rgb(0, 80, 0); color: white; padding: 5px; }"
                     "QPushButton:hover { background-color: rgb(0, 120, 0); }");

        // Accept keyboard focus
        setFocusPolicy(Qt::StrongFocus);
        tab_widget->setFocusPolicy(Qt::StrongFocus);
    }

    // Load HTML help file and parse into tabs
    bool loadHelpFile(const char *filename, const char *fallback_text) {
        QString html_content;

        // STEP 1: Try Qt embedded resource first (compiled-in help files)
        QString resource_path = QString(":/help/%1").arg(filename);
        QFile resource_file(resource_path);
        if (resource_file.exists() && resource_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&resource_file);
            html_content = in.readAll();
            resource_file.close();
#if DEBUG_HELP
            printf("Loaded help file from resource: %s\n", resource_path.toUtf8().constData());
#endif
            // Parse HTML and split by <h1> tags
            return parseAndCreateTabs(html_content);
        }

        // STEP 2: Try multiple file system search paths (fallback for development/modding)
        const char *search_paths[] = {
            "games/%s",           // Game-specific help files
            "../games/%s",
            "../../games/%s",
            "%s",                 // Try exact path
            nullptr
        };

        bool file_found = false;
        for (int i = 0; search_paths[i]; i++) {
            char path[256];
            snprintf(path, sizeof(path), search_paths[i], filename);

            QFile file(path);
            if (file.exists() && file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                html_content = in.readAll();
                file.close();
                file_found = true;
#if DEBUG_HELP
                printf("Loaded help file from file: %s\n", path);
#endif
                break;
            }
        }

        if (!file_found) {
            // Fall back to provided text
            html_content = QString::fromUtf8(fallback_text);
#if DEBUG_HELP
            printf("Using fallback help text (file not found: %s)\n", filename);
#endif
        }

        // Parse HTML and split by <h1> tags
        return parseAndCreateTabs(html_content);
    }

protected:
    // Handle keyboard events
    void keyPressEvent(QKeyEvent *event) override {
        int key = event->key();

        // ESC or Q closes help
        if (key == Qt::Key_Escape || key == Qt::Key_Q || key == Qt::Key_F1) {
            closeHelp();
            return;
        }

        // Number keys 1-9 switch tabs
        if (key >= Qt::Key_1 && key <= Qt::Key_9) {
            int tab_index = key - Qt::Key_1;
            if (tab_index < tab_widget->count()) {
                tab_widget->setCurrentIndex(tab_index);
            }
            return;
        }

        // Arrow keys navigate tabs
        if (key == Qt::Key_Left) {
            int current = tab_widget->currentIndex();
            if (current > 0) {
                tab_widget->setCurrentIndex(current - 1);
            }
            return;
        }
        if (key == Qt::Key_Right) {
            int current = tab_widget->currentIndex();
            if (current < tab_widget->count() - 1) {
                tab_widget->setCurrentIndex(current + 1);
            }
            return;
        }

        // Space or Backspace also closes help
        if (key == Qt::Key_Space || key == Qt::Key_Backspace) {
            closeHelp();
            return;
        }

        // Let the tab widget handle other keys (like page up/down for scrolling)
        QWidget::keyPressEvent(event);
    }

    void closeHelp() {
        // Hide the help overlay
        hide();

        // Set flag
        extern int help_is_showing;
        help_is_showing = 0;

        // Restore centered picture if one was saved
        if (saved_centered_pic) {
            extern void set_centered_pic(Picture *);
            set_centered_pic(saved_centered_pic);
            saved_centered_pic = nullptr;
        }

        // Give focus back to parent (game window)
        if (parentWidget()) {
            parentWidget()->setFocus();
        }

        // Trigger full redraw including centered picture if present
        // This will call redraw_cb() AND redraw centered picture on top
        extern void invalidate(int, int, int, int);
        invalidate(0, 0, table_width, table_height);

#if DEBUG_HELP
        printf("Help closed\n");
#endif
    }

private:
    QTabWidget *tab_widget;

    // Load and embed images from game's image system into HTML
    QString processImagesInHTML(const QString &html) {
        QString processed_html = html;

        // Find all <img src=...> tags (original format doesn't use quotes)
        // Matches: <img src=name>, <img align=left src=name>, etc.
        QRegularExpression img_regex("<img[^>]*src=([^ >]+)[^>]*>",
                                     QRegularExpression::CaseInsensitiveOption);

        int pos = 0;
        QRegularExpressionMatch match;
        while ((match = img_regex.match(processed_html, pos)).hasMatch()) {
            pos = match.capturedStart();
            QString full_tag = match.captured(0);
            QString img_name = match.captured(1);

#if DEBUG_HELP
            printf("Found image tag: %s (name: %s)\n",
                   full_tag.toUtf8().constData(),
                   img_name.toUtf8().constData());
#endif

            // Try to load the image using get_picture() which handles both
            // regular images and synthesized cards (like "as" for ace of spades)
            extern Picture *get_picture(char *);
            Picture *pic = get_picture(const_cast<char*>(img_name.toUtf8().constData()));
            image *img = (image *)pic;

            // If image has a synthesis function, call it to generate the graphics
            if (img && img->synth_func) {
                img->synth_func(img);
            }

            if (img && img->pixels) {
                // Convert to QPixmap
                typedef struct {
                    QPixmap *pixmap;
                    QPixmap *mask;
                    QPixmap *rotated;
                    QPixmap *inverted;
                } qt_pixels_type;

                qt_pixels_type *qp = (qt_pixels_type *)img->pixels;
                if (qp && qp->pixmap) {
                    // Convert QPixmap to base64 data URI for embedding
                    QByteArray byte_array;
                    QBuffer buffer(&byte_array);
                    buffer.open(QIODevice::WriteOnly);
                    qp->pixmap->save(&buffer, "PNG");
                    QString base64_img = QString("data:image/png;base64,") + byte_array.toBase64();

                    // Replace src=name with src="data:..."
                    QString new_tag = full_tag;
                    QRegularExpression src_regex("src=" + QRegularExpression::escape(img_name),
                                                 QRegularExpression::CaseInsensitiveOption);
                    new_tag.replace(src_regex, QString("src=\"") + base64_img + "\"");

                    processed_html.replace(pos, full_tag.length(), new_tag);
                    pos += new_tag.length();

#if DEBUG_HELP
                    printf("  Embedded image: %s (%dx%d)\n",
                           img_name.toUtf8().constData(),
                           qp->pixmap->width(), qp->pixmap->height());
#endif
                } else {
                    pos += full_tag.length();
                }
            } else {
                // Image not found, leave tag as-is
#if DEBUG_HELP
                printf("  Image not found: %s\n", img_name.toUtf8().constData());
#endif
                pos += full_tag.length();
            }
        }

        return processed_html;
    }

    // Parse HTML content and create tabs based on <h1> sections
    bool parseAndCreateTabs(const QString &html) {
        // Clear existing tabs
        while (tab_widget->count() > 0) {
            tab_widget->removeTab(0);
        }

        // Find all <h1> sections
        QStringList sections;
        QStringList titles;

        int pos = 0;
        int last_h1_end = 0;

        // Check if there's content before the first <h1> (intro section)
        int first_h1 = html.indexOf("<h1>", 0, Qt::CaseInsensitive);
        if (first_h1 > 0) {
            QString intro = html.mid(0, first_h1).trimmed();
            if (!intro.isEmpty()) {
                titles.append("Introduction");
                sections.append(intro);
            }
        }

        // Parse <h1> sections
        while (pos < html.length()) {
            int h1_start = html.indexOf("<h1>", pos, Qt::CaseInsensitive);
            if (h1_start < 0) break;

            int h1_end = html.indexOf("</h1>", h1_start, Qt::CaseInsensitive);
            if (h1_end < 0) break;

            // Extract title
            QString title = html.mid(h1_start + 4, h1_end - h1_start - 4).trimmed();

            // Find next <h1> or end of document
            int next_h1 = html.indexOf("<h1>", h1_end, Qt::CaseInsensitive);
            int content_end = (next_h1 > 0) ? next_h1 : html.length();

            // Extract content (everything between </h1> and next <h1>)
            QString content = html.mid(h1_end + 5, content_end - h1_end - 5).trimmed();

            if (!title.isEmpty()) {
                titles.append(title);
                sections.append(content);
            }

            pos = h1_end + 5;
        }

        // If no sections found, create a single tab with all content
        if (sections.isEmpty()) {
            titles.append("Help");
            sections.append(html);
        }

#if DEBUG_HELP
        printf("Parsed %d help sections\n", sections.size());
#endif

        // Create a tab for each section
        for (int i = 0; i < sections.size(); i++) {
            QTextBrowser *browser = new QTextBrowser();
            browser->setOpenExternalLinks(true);
            // Must be set before setHtml() to take effect
            browser->document()->setDefaultStyleSheet("a { color: yellow; }");

            // Process and embed images before setting HTML
            QString processed_html = processImagesInHTML(sections[i]);
            browser->setHtml(processed_html);

            // Set font size for readability
            QFont font = browser->font();
            font.setPointSize(10);
            browser->setFont(font);

            tab_widget->addTab(browser, titles[i]);

#if DEBUG_HELP
            printf("  Tab %d: %s\n", i, titles[i].toUtf8().constData());
#endif
        }

        // Add statistics tab at the end
        QTextBrowser *stats_browser = new QTextBrowser();
        stats_browser->setOpenExternalLinks(false);

        // Get game statistics
        int starts = 0, wins = 0;
        score_get_stats(&starts, &wins);

        // Calculate win percentage
        double percentage = 0.0;
        if (starts > 0) {
            percentage = (double)wins / (double)starts * 100.0;
        }

        // Format statistics as HTML
        QString stats_html = QString(
            "<html><body style='color: white;'>"
            "<h2 style='text-align: center;'>Game Statistics</h2>"
            "<br><br>"
            "<p style='font-size: 14pt;'><b>Game Starts:</b> %1</p>"
            "<p style='font-size: 14pt;'><b>Game Wins:</b> %2</p>"
            "<p style='font-size: 14pt;'><b>Percentage of wins:</b> %3%</p>"
            "</body></html>"
        ).arg(starts).arg(wins).arg(percentage, 0, 'f', 1);

        stats_browser->setHtml(stats_html);

        // Set font size for readability
        QFont stats_font = stats_browser->font();
        stats_font.setPointSize(10);
        stats_browser->setFont(stats_font);

        tab_widget->addTab(stats_browser, "Stats");

#if DEBUG_HELP
        printf("  Tab %d: Stats (starts=%d, wins=%d, %.1f%%)\n",
               sections.size(), starts, wins, percentage);
#endif

        return sections.size() > 0;
    }
};

// Global help widget instance (accessed from qwin.cpp for resize handling)
HelpWidget *global_help_widget = nullptr;

// ============================================================================
// PUBLIC API FUNCTIONS (extern "C")
// ============================================================================

extern "C" void help(char *filename)
{
#if DEBUG_HELP
    printf("help() called: filename=%s\n", filename ? filename : "(null)");
#endif

    if (!qt_window) {
        printf("ERROR: help() called but qt_window is null\n");
        return;
    }

    // Save the last cleared centered picture (splash, youwin, youlose, etc.)
    // Games call set_centered_pic(0) right before calling help(), so get_centered_pic() returns NULL
    // but qtable.cpp saved the old value in last_cleared_picture
    extern Picture *last_cleared_picture;
    saved_centered_pic = last_cleared_picture;
#if DEBUG_HELP
    printf("help() saved last cleared pic: %p\n", saved_centered_pic);
#endif

    // Create help widget if it doesn't exist
    if (!global_help_widget) {
        global_help_widget = new HelpWidget(qt_window);
    }

    // Load the help content with simple fallback text
    const char *default_text =
        "Help\n\n"
        "Press h for help.\n"
        "Press backspace to undo.\n"
        "Press q to quit.\n";
    if (!global_help_widget->loadHelpFile(filename, default_text)) {
        printf("ERROR: Failed to load help content\n");
        return;
    }

    // Resize to match parent window
    global_help_widget->resize(qt_window->size());

    // Show the overlay
    global_help_widget->show();
    global_help_widget->raise();  // Bring to front
    global_help_widget->setFocus();

    // Set the help_is_showing flag
    extern int help_is_showing;
    help_is_showing = 1;

#if DEBUG_HELP
    printf("Help widget shown\n");
#endif
}
