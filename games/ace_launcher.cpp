/* Ace of Penguins Game Launcher - Qt5/Qt6
   Provides a simple GUI to launch any of the solitaire games
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
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QProcess>
#include <QFont>
#include <QPixmap>
#include <QPalette>
#include <QIcon>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScrollArea>
#include <QScreen>
#include <QGuiApplication>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDomDocument>
#include <QCoreApplication>
#include <QList>
#include <algorithm>

extern "C" {
#include "imagelib.h"
#include "cards.h"
#include "xwin.h"
}

// Qt Pixmap type for images (must match definition in qwin.cpp)
typedef struct {
    QPixmap *image_pixmap;
    QPixmap *image_mask;
    QPixmap *rotated_pixmap;
    QPixmap *rotated_mask;
    QPixmap *inverted_pixmap;
} qt_pixels_type;

// Game Stats popup window
class GameStatsWindow : public QWidget {
public:
    GameStatsWindow(QWidget *parent = nullptr) : QWidget(parent) {
        setWindowTitle("Game Stats");
        setWindowFlags(Qt::Window);  // Separate window, not embedded

        // Set window icon to ace of spades
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

        // Set green background
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(0x00, 0x66, 0x00));
        setAutoFillBackground(true);
        setPalette(palette);

        // Create main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // Create table widget
        QTableWidget *statsTable = new QTableWidget();
        statsTable->setColumnCount(4);
        QStringList headers;
        headers << "Game" << "Game Starts" << "Game Wins" << "Percentage of Wins";
        statsTable->setHorizontalHeaderLabels(headers);

        // Style the table for visibility on green background
        statsTable->setStyleSheet(
            "QTableWidget { background-color: white; color: black; }"
            "QHeaderView::section { background-color: rgb(200, 200, 200); color: black; font-weight: bold; }");

        // Load and populate stats from XML
        loadGameStats(statsTable);

        // Adjust column widths
        statsTable->resizeColumnsToContents();
        statsTable->horizontalHeader()->setStretchLastSection(false);

        // Set table size policy to prefer its content size
        statsTable->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

        mainLayout->addWidget(statsTable);

        // Automatically resize window to fit contents
        adjustSize();

        // Get screen size and limit window if needed
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int maxWidth = screenGeometry.width() - 100;
        int maxHeight = screenGeometry.height() - 100;

        if (width() > maxWidth || height() > maxHeight) {
            resize(qMin(width(), maxWidth), qMin(height(), maxHeight));
        }
    }

private:
    void loadGameStats(QTableWidget *table) {
        // Determine XML file path (same logic as qscore.cpp)
        QString xmlFilePath;
#ifdef _WIN32
        // Windows: Same directory as executable
        QString exePath = QCoreApplication::applicationFilePath();
        QFileInfo info(exePath);
        QString dir = info.absolutePath();
        xmlFilePath = dir + "/GameScores.xml";
#else
        // Linux: User's home directory
        QString homeDir = QDir::homePath();
        xmlFilePath = homeDir + "/GameScores.xml";
#endif

        // Try to open the XML file
        QFile file(xmlFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            // File doesn't exist or can't be read - just return without crashing
            return;
        }

        // Parse XML
        QDomDocument doc;
        if (!doc.setContent(&file)) {
            // Invalid XML - just return without crashing
            file.close();
            return;
        }
        file.close();

        // Get all <Game> elements
        QDomNodeList gameNodes = doc.elementsByTagName("Game");
        table->setRowCount(gameNodes.count() + 1);  // +1 for totals row

        if (gameNodes.count() == 0) {
            // No games found - just return without crashing
            return;
        }

        int totalStarts = 0;
        int totalWins = 0;

        struct GameStat {
            QString name;
            int starts;
            int wins;
        };
        QList<GameStat> stats;

        for (int i = 0; i < gameNodes.count(); i++) {
            QDomElement gameElement = gameNodes.at(i).toElement();

            QDomNodeList startsNodes = gameElement.elementsByTagName("NumberOfStarts");
            QDomNodeList winsNodes = gameElement.elementsByTagName("NumberOfWins");

            GameStat stat;
            stat.name = gameElement.attribute("name", "Unknown");
            stat.starts = startsNodes.isEmpty() ? 0 : startsNodes.at(0).toElement().text().toInt();
            stat.wins = winsNodes.isEmpty() ? 0 : winsNodes.at(0).toElement().text().toInt();
            stats.append(stat);
        }

        std::sort(stats.begin(), stats.end(), [](const GameStat &a, const GameStat &b) {
            return a.name.compare(b.name, Qt::CaseInsensitive) < 0;
        });

        for (int i = 0; i < stats.size(); i++) {
            const QString &gameName = stats[i].name;
            int starts = stats[i].starts;
            int wins = stats[i].wins;

            // Accumulate totals
            totalStarts += starts;
            totalWins += wins;

            // Calculate percentage
            double percentage = (starts > 0) ? (wins * 100.0 / starts) : 0.0;
            QString percentageStr = QString::number(percentage, 'f', 1) + "%";

            // Add to table
            table->setItem(i, 0, new QTableWidgetItem(gameName));
            table->setItem(i, 1, new QTableWidgetItem(QString::number(starts)));
            table->setItem(i, 2, new QTableWidgetItem(QString::number(wins)));
            table->setItem(i, 3, new QTableWidgetItem(percentageStr));
        }

        // Add totals row
        int totalsRow = gameNodes.count();
        double totalPercentage = (totalStarts > 0) ? (totalWins * 100.0 / totalStarts) : 0.0;
        QString totalPercentageStr = QString::number(totalPercentage, 'f', 1) + "%";

        QTableWidgetItem *totalLabel = new QTableWidgetItem("TOTAL");
        QTableWidgetItem *totalStartsItem = new QTableWidgetItem(QString::number(totalStarts));
        QTableWidgetItem *totalWinsItem = new QTableWidgetItem(QString::number(totalWins));
        QTableWidgetItem *totalPercentageItem = new QTableWidgetItem(totalPercentageStr);

        // Make totals row bold
        QFont boldFont;
        boldFont.setBold(true);
        totalLabel->setFont(boldFont);
        totalStartsItem->setFont(boldFont);
        totalWinsItem->setFont(boldFont);
        totalPercentageItem->setFont(boldFont);

        table->setItem(totalsRow, 0, totalLabel);
        table->setItem(totalsRow, 1, totalStartsItem);
        table->setItem(totalsRow, 2, totalWinsItem);
        table->setItem(totalsRow, 3, totalPercentageItem);
    }
};

// Info overlay widget (similar to HelpWidget in qhelp.cpp)
class InfoOverlay : public QWidget {
public:
    InfoOverlay(QWidget *parent = nullptr) : QWidget(parent) {
        // Set up overlay properties
        setWindowFlags(Qt::Widget);  // Child widget, not a separate window
        setAttribute(Qt::WA_StyledBackground);

        // Create main layout
        QVBoxLayout *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // Top section with Credits label and card image (clickable to close)
        QHBoxLayout *topLayout = new QHBoxLayout();

        // Credits label on the left
        QLabel *creditsLabel = new QLabel("Credits");
        QFont titleFont("Arial", 20, QFont::Bold);
        creditsLabel->setFont(titleFont);
        creditsLabel->setStyleSheet("color: white;");
        topLayout->addWidget(creditsLabel);

        topLayout->addStretch();

        // Card button (clickable to close overlay)
        QPushButton *cardButton = new QPushButton();
        cardButton->setFlat(true);
        cardButton->setCursor(Qt::PointingHandCursor);

        // Get the synthesized "as" (ace of spades) card image
        Picture *card_img = get_picture(const_cast<char*>("as"));
        if (card_img && card_img->w > 0) {
            image *img = reinterpret_cast<image*>(card_img);
            if (img && img->pixels) {
                qt_pixels_type *qp = reinterpret_cast<qt_pixels_type*>(img->pixels);
                if (qp && qp->image_pixmap) {
                    cardButton->setIcon(QIcon(*qp->image_pixmap));
                    cardButton->setIconSize(qp->image_pixmap->size());
                }
            }
        }

        connect(cardButton, &QPushButton::clicked, this, &InfoOverlay::closeOverlay);
        topLayout->addWidget(cardButton);

        mainLayout->addLayout(topLayout);

        // Placeholder text
        QLabel *placeholderLabel = new QLabel(
            "Credit first and foremost goes to DJ Delorie for creating the Ace of Penguins "
            "games and libraries.  Additional credit also goes to his wife Pat for inspring the "
            "creation of these games as well as endless hours of testing.\n\n"
            "Additional credit to contributors:\n"
            "Martin Thornquist   (Canfield and Thornq)\n"
            "Eric Bachalo   (Spider)\n"
            "Brian Rater\n"
            "Thimo Neubauer\n"
            "Mark Arrasmith\n"
            "Jari Aalto\n\n"
            "And of course credit to me, Chuck Bown.  I did the X11 to Qt graphics library port, the "
            "persistent score keeping, Yukon, as well as the game launcher.\n"
            "I've loved and played these games for decades.  And now with the Qt port these "
            "games can be enjoyed on either linux or windows.\n\n"
            "Official Github for version 2.0:\nhttps://github.com/ChuckBown/Ace_of_Penguins_Solitaire_v2.0"
        );
        placeholderLabel->setStyleSheet("color: white; font-size: 14pt;");
        placeholderLabel->setWordWrap(true);
        mainLayout->addWidget(placeholderLabel);

        mainLayout->addStretch();

        // Set green background and white text
        setStyleSheet("QWidget { background-color: rgb(0, 102, 0); }");

        // Accept keyboard focus
        setFocusPolicy(Qt::StrongFocus);
    }

    void showOverlay() {
        // Resize to fill parent
        if (parentWidget()) {
            setGeometry(parentWidget()->rect());
        }
        show();
        raise();
        setFocus();
    }

    void closeOverlay() {
        hide();
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        // ESC or Q to close
        if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Q) {
            closeOverlay();
        }
    }

    void resizeEvent(QResizeEvent *event) override {
        // Keep overlay covering the entire parent when resized
        if (parentWidget()) {
            setGeometry(parentWidget()->rect());
        }
        QWidget::resizeEvent(event);
    }
};

class LauncherWindow : public QWidget {
public:
    LauncherWindow(QWidget *parent = nullptr) : QWidget(parent), overlay(nullptr), statsWindow(nullptr) {
        setWindowTitle("Ace of Penguins Solitaire");

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

        // Set green background color matching the games (RGB 0, 102, 0)
        QPalette palette;
        palette.setColor(QPalette::Window, QColor(0x00, 0x66, 0x00));
        setAutoFillBackground(true);
        setPalette(palette);

        setupUI();

        // Create overlay widget
        overlay = new InfoOverlay(this);
        overlay->hide();
    }

private:
    void setupUI() {
        // Create a container widget for all content
        QWidget *contentWidget = new QWidget();

        // Set green background color on content widget (RGB 0, 102, 0)
        QPalette contentPalette;
        contentPalette.setColor(QPalette::Window, QColor(0x00, 0x66, 0x00));
        contentWidget->setAutoFillBackground(true);
        contentWidget->setPalette(contentPalette);

        // Main vertical layout for content
        QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
        mainLayout->setContentsMargins(20, 20, 20, 20);
        mainLayout->setSpacing(15);

        // Top section with title and card
        QHBoxLayout *topLayout = new QHBoxLayout();

        // Title label
        QLabel *titleLabel = new QLabel("Ace of Penguins Solitaire\nGame Launcher");
        QFont titleFont("Arial", 20, QFont::Bold);
        titleLabel->setFont(titleFont);
        titleLabel->setStyleSheet("color: white;");
        topLayout->addWidget(titleLabel);

        topLayout->addStretch();

        // Card graphic (ace of spades) - as a clickable button
        QPushButton *cardButton = new QPushButton();
        cardButton->setFlat(true);  // Remove button border/background
        cardButton->setCursor(Qt::PointingHandCursor);  // Show hand cursor on hover

        // Get the synthesized "as" (ace of spades) card image
        Picture *card_img = get_picture(const_cast<char*>("as"));
        if (card_img && card_img->w > 0) {
            // Picture is actually an image* cast
            image *img = reinterpret_cast<image*>(card_img);

            // Access the Qt pixmap through the pixels field (like qhelp.cpp does)
            if (img && img->pixels) {
                qt_pixels_type *qp = reinterpret_cast<qt_pixels_type*>(img->pixels);
                if (qp && qp->image_pixmap) {
                    cardButton->setIcon(QIcon(*qp->image_pixmap));
                    cardButton->setIconSize(qp->image_pixmap->size());
                } else {
                    cardButton->setText("[Penguin]");
                    cardButton->setStyleSheet("border: 2px solid black; padding: 20px; background-color: white;");
                }
            } else {
                cardButton->setText("[Penguin]");
                cardButton->setStyleSheet("border: 2px solid black; padding: 20px; background-color: white;");
            }
        } else {
            // Fallback: just show a placeholder
            cardButton->setText("[Penguin]");
            cardButton->setStyleSheet("border: 2px solid black; padding: 20px; background-color: white;");
        }

        // Connect click handler (functionality to be added later)
        connect(cardButton, &QPushButton::clicked, [this]() {
            onCardButtonClicked();
        });

        topLayout->addWidget(cardButton);
        mainLayout->addLayout(topLayout);

        // Card Games section
        QLabel *cardGamesHeader = new QLabel("Card Games:");
        QFont headerFont("Arial", 14, QFont::Bold);
        cardGamesHeader->setFont(headerFont);
        cardGamesHeader->setStyleSheet("color: white; margin-top: 10px;");
        mainLayout->addWidget(cardGamesHeader);

        // Create frame for card game buttons
        QFrame *cardGamesFrame = new QFrame();
        cardGamesFrame->setStyleSheet("QFrame { border: 2px solid white; background-color: rgba(0, 0, 0, 0); }");
        QVBoxLayout *cardGamesLayout = new QVBoxLayout(cardGamesFrame);
        cardGamesLayout->setContentsMargins(10, 10, 10, 10);
        cardGamesLayout->setSpacing(10);

        // Card game buttons (alphabetical order)
        addGameButton(cardGamesLayout, "Canfield", "canfield");
        addGameButton(cardGamesLayout, "Freecell", "freecell");
        addGameButton(cardGamesLayout, "Golf", "golf");
        addGameButton(cardGamesLayout, "Solitaire (Klondike)", "solitaire");
        addGameButton(cardGamesLayout, "Spider", "spider");
        addGameButton(cardGamesLayout, "Thornq (Scorpion)", "thornq");
        addGameButton(cardGamesLayout, "Yukon", "yukon");

        mainLayout->addWidget(cardGamesFrame);

        // Non-Card Games section
        QLabel *nonCardGamesHeader = new QLabel("Non-Card Games:");
        nonCardGamesHeader->setFont(headerFont);
        nonCardGamesHeader->setStyleSheet("color: white; margin-top: 10px;");
        mainLayout->addWidget(nonCardGamesHeader);

        // Create frame for non-card game buttons
        QFrame *nonCardGamesFrame = new QFrame();
        nonCardGamesFrame->setStyleSheet("QFrame { border: 2px solid white; background-color: rgba(0, 0, 0, 0); }");
        QVBoxLayout *nonCardGamesLayout = new QVBoxLayout(nonCardGamesFrame);
        nonCardGamesLayout->setContentsMargins(10, 10, 10, 10);
        nonCardGamesLayout->setSpacing(10);

        // Non-card game buttons (alphabetical order)
        addGameButton(nonCardGamesLayout, "Mastermind", "mastermind");
        addGameButton(nonCardGamesLayout, "Merlin", "merlin");
        addGameButton(nonCardGamesLayout, "Minesweeper", "minesweeper");
        addGameButton(nonCardGamesLayout, "Pegged", "pegged");
        addGameButton(nonCardGamesLayout, "Taipei (Mahjong)", "taipei");

        mainLayout->addWidget(nonCardGamesFrame);

        // Utility section
        QLabel *utilityHeader = new QLabel("Utility:");
        utilityHeader->setFont(headerFont);
        utilityHeader->setStyleSheet("color: white; margin-top: 10px;");
        mainLayout->addWidget(utilityHeader);

        // Create frame for utility buttons
        QFrame *utilityFrame = new QFrame();
        utilityFrame->setStyleSheet("QFrame { border: 2px solid white; background-color: rgba(0, 0, 0, 0); }");
        QVBoxLayout *utilityLayout = new QVBoxLayout(utilityFrame);
        utilityLayout->setContentsMargins(10, 10, 10, 10);
        utilityLayout->setSpacing(10);

        // Utility buttons
        addGameButton(utilityLayout, "Taipei Layout Editor", "taipedit");

        QPushButton *statsButton = addButton(utilityLayout, "Game Stats");
        connect(statsButton, &QPushButton::clicked, [this]() {
            showGameStats();
        });

        mainLayout->addWidget(utilityFrame);

        mainLayout->addStretch();

        // Create scroll area and set the content widget
        QScrollArea *scrollArea = new QScrollArea();
        scrollArea->setWidget(contentWidget);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

        // Make scroll area background transparent so contentWidget background shows
        scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }");

        // Set scroll area as the main layout
        QVBoxLayout *windowLayout = new QVBoxLayout(this);
        windowLayout->setContentsMargins(0, 0, 0, 0);
        windowLayout->addWidget(scrollArea);
        setLayout(windowLayout);

        // Set window size
        setMinimumWidth(450);
        setMinimumHeight(300);

        // Set initial window size based on content size
        // Force layout to calculate its size
        contentWidget->adjustSize();
        QSize contentSize = contentWidget->sizeHint();

        // Get available screen height (excludes taskbar/menu bar)
        QScreen *screen = QGuiApplication::primaryScreen();
        int availableHeight = screen->availableGeometry().height();

        // Calculate desired window height (content + buffer)
        int desiredHeight = contentSize.height() + 20;

        // Limit window height to available screen height
        int windowHeight = (desiredHeight < availableHeight) ? desiredHeight : availableHeight;

        // Set initial window size
        resize(contentSize.width() + 20, windowHeight);

        // Position window at top of screen (y=0)
        QRect screenGeometry = screen->availableGeometry();
        move(screenGeometry.x() + 100, screenGeometry.y());
    }

    QPushButton *addButton(QVBoxLayout *layout, const QString &label) {
        QPushButton *button = new QPushButton(label);
        button->setMinimumHeight(35);
        QFont buttonFont("Arial", 12);
        button->setFont(buttonFont);

        // Set button colors (light gray background with black text, light green on hover)
        button->setStyleSheet(
            "QPushButton { background-color: rgb(235, 235, 235); color: black; }"
            "QPushButton:hover { background-color: rgb(180, 255, 180); color: black; }");

        layout->addWidget(button);
        return button;
    }

    void addGameButton(QVBoxLayout *layout, const QString &gameName, const QString &executable) {
        QPushButton *button = addButton(layout, gameName);
        connect(button, &QPushButton::clicked, [this, executable]() {
            launchGame(executable);
        });
    }

    void launchGame(const QString &executable) {
        // Launch the game executable from the same directory as ace_launcher
        // Use applicationDirPath() so it works even when launched from desktop shortcuts
        QString executableDir = QCoreApplication::applicationDirPath();
        QString execPath = executableDir + "/" + executable;

        bool success = QProcess::startDetached(execPath, QStringList());

        if (!success) {
            QMessageBox::warning(this, "Launch Error",
                QString("Failed to launch %1.\nMake sure the executable exists in: %2").arg(executable, executableDir));
        }
    }

    void onCardButtonClicked() {
        // Toggle the overlay
        if (overlay && overlay->isVisible()) {
            overlay->closeOverlay();
        } else if (overlay) {
            overlay->showOverlay();
        }
    }

    void showGameStats() {
        // Delete old window if it exists (to refresh data)
        if (statsWindow) {
            delete statsWindow;
            statsWindow = nullptr;
        }
        // Create fresh window (loads current XML data)
        statsWindow = new GameStatsWindow(this);
        statsWindow->show();
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        QWidget::resizeEvent(event);
        // Keep overlay sized to match parent window
        if (overlay && overlay->isVisible()) {
            overlay->setGeometry(rect());
        }
    }

private:
    InfoOverlay *overlay;
    GameStatsWindow *statsWindow;
};

int main(int argc, char *argv[]) {
    // Suppress common Qt platform warnings that don't affect functionality
    // (XInput2, XKeyboard, OpenGL warnings on minimal X servers)
    qputenv("QT_LOGGING_RULES", "qt.qpa.xcb=false");

    QApplication app(argc, argv);

    // Initialize Qt resources for card images
    Q_INIT_RESOURCE(common);

    // Initialize the card library (minimal initialization for card rendering)
    xwin_init(argc, argv);

    // Initialize Ace library - registers card synthesis and image libraries
    init_ace(argc, argv, nullptr);

    // Register app image library
    register_imagelib(appimglib_imagelib);

    LauncherWindow launcher;
    launcher.show();

    return app.exec();
}
