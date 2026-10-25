/* The Ace of Penguins - qscore.cpp
   Game scorekeeping system using Qt XML handling
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

#include <QFile>
#include <QDomDocument>
#include <QDomElement>
#include <QTextStream>
#include <QFileInfo>
#include <QCoreApplication>
#include <QString>
#include <QDir>
#include <cstdio>

extern "C" {
#include "qscore.h"
}

// Current game name (extracted from argv[0])
static QString current_game_name;

// Path to scores file (same directory as executable)
static QString scores_file_path;

// Extract game name from argv[0] (like we do for window title)
static QString extract_game_name(const char *argv0) {
    if (!argv0) return "Unknown";

    QString full_path = QString::fromUtf8(argv0);

    // Extract just the executable name without path
    int slash_pos = full_path.lastIndexOf('/');
    if (slash_pos == -1) {
        slash_pos = full_path.lastIndexOf('\\'); // Windows path
    }
    QString name = full_path.mid(slash_pos + 1);

    // Remove .exe extension
    name.replace(".exe", "", Qt::CaseInsensitive);

    // Capitalize first letter
    if (name.length() > 0) {
        name[0] = name[0].toUpper();
    }

    return name;
}

// Load XML document from file
static QDomDocument load_scores_xml() {
    QDomDocument doc;

    QFile file(scores_file_path);
    if (!file.exists()) {
        // Create new empty document
        QDomElement root = doc.createElement("GameScores");
        doc.appendChild(root);
        return doc;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        fprintf(stderr, "Warning: Could not open scores file for reading: %s\n",
                scores_file_path.toUtf8().constData());
        QDomElement root = doc.createElement("GameScores");
        doc.appendChild(root);
        return doc;
    }

    QByteArray xmlData = file.readAll();
    file.close();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    // Qt6: use new ParseResult API
    auto parseResult = doc.setContent(xmlData);
    if (!parseResult) {
        fprintf(stderr, "Warning: Error parsing scores XML at line %lld, column %lld: %s\n",
                parseResult.errorLine, parseResult.errorColumn, parseResult.errorMessage.toUtf8().constData());
        // Return empty document
        doc.clear();
        QDomElement root = doc.createElement("GameScores");
        doc.appendChild(root);
        return doc;
    }
#else
    // Qt5: use old bool + out-parameters API
    QString errorMsg;
    int errorLine, errorColumn;
    if (!doc.setContent(xmlData, &errorMsg, &errorLine, &errorColumn)) {
        fprintf(stderr, "Warning: Error parsing scores XML at line %d, column %d: %s\n",
                errorLine, errorColumn, errorMsg.toUtf8().constData());
        // Return empty document
        doc.clear();
        QDomElement root = doc.createElement("GameScores");
        doc.appendChild(root);
        return doc;
    }
#endif
    return doc;
}

// Save XML document to file
static bool save_scores_xml(const QDomDocument &doc) {
    QFile file(scores_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        fprintf(stderr, "Warning: Could not open scores file for writing: %s\n",
                scores_file_path.toUtf8().constData());
        return false;
    }

    QTextStream stream(&file);
    stream << doc.toString(2); // Indent with 2 spaces
    file.close();
    return true;
}

// Find or create game element in XML
static QDomElement find_or_create_game(QDomDocument &doc, const QString &game_name) {
    QDomElement root = doc.documentElement();
    if (root.isNull()) {
        root = doc.createElement("GameScores");
        doc.appendChild(root);
    }

    // Search for existing game entry
    QDomNodeList games = root.elementsByTagName("Game");
    for (int i = 0; i < games.count(); i++) {
        QDomElement game = games.at(i).toElement();
        if (game.attribute("name") == game_name) {
            return game;
        }
    }

    // Create new game entry
    QDomElement game = doc.createElement("Game");
    game.setAttribute("name", game_name);

    QDomElement starts = doc.createElement("NumberOfStarts");
    starts.appendChild(doc.createTextNode("0"));
    game.appendChild(starts);

    QDomElement wins = doc.createElement("NumberOfWins");
    wins.appendChild(doc.createTextNode("0"));
    game.appendChild(wins);

    root.appendChild(game);
    return game;
}

// Increment a counter in the game element
static void increment_counter(const QString &tag_name) {
    if (current_game_name.isEmpty()) {
        fprintf(stderr, "Warning: score_init() not called before incrementing counter\n");
        return;
    }

    QDomDocument doc = load_scores_xml();
    QDomElement game = find_or_create_game(doc, current_game_name);

    // Find the counter element
    QDomNodeList nodes = game.elementsByTagName(tag_name);
    if (nodes.isEmpty()) {
        // Create it if it doesn't exist
        QDomElement counter = doc.createElement(tag_name);
        counter.appendChild(doc.createTextNode("1"));
        game.appendChild(counter);
    } else {
        QDomElement counter = nodes.at(0).toElement();
        int value = counter.text().toInt();
        value++;

        // Replace text node
        while (!counter.firstChild().isNull()) {
            counter.removeChild(counter.firstChild());
        }
        counter.appendChild(doc.createTextNode(QString::number(value)));
    }

    save_scores_xml(doc);
}

// Decrement a counter in the game element
static void decrement_counter(const QString &tag_name) {
    if (current_game_name.isEmpty()) {
        fprintf(stderr, "Warning: score_init() not called before decrementing counter\n");
        return;
    }

    QDomDocument doc = load_scores_xml();
    QDomElement game = find_or_create_game(doc, current_game_name);

    // Find the counter element
    QDomNodeList nodes = game.elementsByTagName(tag_name);
    if (nodes.isEmpty()) {
        // Counter doesn't exist, nothing to decrement
        return;
    } else {
        QDomElement counter = nodes.at(0).toElement();
        int value = counter.text().toInt();
        if (value > 0) {
            value--;
        }

        // Replace text node
        while (!counter.firstChild().isNull()) {
            counter.removeChild(counter.firstChild());
        }
        counter.appendChild(doc.createTextNode(QString::number(value)));
    }

    save_scores_xml(doc);
}

// ============================================================================
// PUBLIC API FUNCTIONS (extern "C")
// ============================================================================

extern "C" void score_init(const char *argv0) {
    // Extract game name from argv[0]
    current_game_name = extract_game_name(argv0);

    // Determine scores file path
    // Windows: Same directory as executable (existing behavior)
    // Linux: User's home directory
#ifdef _WIN32
    // Windows: Use executable directory
    QString exe_path;
    if (argv0) {
        exe_path = QString::fromUtf8(argv0);
        QFileInfo info(exe_path);
        QString dir = info.absolutePath();
        scores_file_path = dir + "/GameScores.xml";
    } else {
        // Fallback to current directory
        scores_file_path = "GameScores.xml";
    }
#else
    // Linux/Unix: Use home directory
    QString home_dir = QDir::homePath();
    scores_file_path = home_dir + "/GameScores.xml";
#endif

#if 0
    printf("Score system initialized: game='%s', file='%s'\n",
           current_game_name.toUtf8().constData(),
           scores_file_path.toUtf8().constData());
#endif
}

extern "C" void score_increment_starts(void) {
    increment_counter("NumberOfStarts");
}

extern "C" void score_decrement_starts(void) {
    decrement_counter("NumberOfStarts");
}

extern "C" void score_increment_wins(void) {
    increment_counter("NumberOfWins");
}

extern "C" int score_get_stats(int *starts, int *wins) {
    if (current_game_name.isEmpty()) {
#if 0
        fprintf(stderr, "Warning: score_init() not called before getting stats\n");
#endif
        return -1;
    }

    QDomDocument doc = load_scores_xml();
    QDomElement game = find_or_create_game(doc, current_game_name);

    QDomNodeList starts_nodes = game.elementsByTagName("NumberOfStarts");
    QDomNodeList wins_nodes = game.elementsByTagName("NumberOfWins");

    if (starts) {
        *starts = starts_nodes.isEmpty() ? 0 : starts_nodes.at(0).toElement().text().toInt();
    }

    if (wins) {
        *wins = wins_nodes.isEmpty() ? 0 : wins_nodes.at(0).toElement().text().toInt();
    }

    return 0;
}
