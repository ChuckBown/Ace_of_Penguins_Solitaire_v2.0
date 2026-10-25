# Ace of Penguins 2.0

A collection of classic one-player card and puzzle games, updated to run on
both **Linux** and **Windows**.

Version 2.0 is a port of the original Ace of Penguins games from X11 to
**Qt** (Qt5 or Qt6), with some new features added along the way.

- **Version 2.0 (this project):** https://github.com/ChuckBown/Ace_of_Penguins_Solitaire_v2.0
- **Original Ace of Penguins v1.4 by DJ Delorie:** http://www.delorie.com/store/ace/

For full details on what changed in version 2.0, including build notes,
install notes, file changes and gameplay changes, see
[ACE_2.0.txt](ACE_2.0.txt).

## The Games

**Card games**

- Canfield
- Freecell
- Golf
- Solitaire (Klondike)
- Spider
- Thornq (Scorpion)
- Yukon *(new in 2.0)*

**Other games**

- Mastermind
- Merlin
- Minesweeper
- Pegged
- Taipei (Mahjong)

**Utilities**

- Taipei Layout Editor (`taipedit`), for creating custom Taipei layouts
- Game Launcher (`ace_launcher`), a single window for starting any of the
  games and viewing your game statistics *(new in 2.0)*

## What's New in 2.0

- Runs on Linux and Windows using Qt5 or Qt6.
- New game: **Yukon**, a cross between Klondike and Thornq.
- Persistent score keeping: every game tracks how many times you have
  started and won. View your stats on each game's Help screen, or for all
  games at once from the launcher's **Game Stats** button.
- Mouse forward/back buttons: forward starts a new game, back undoes the
  last move.
- New options in several games, such as 1-card or 3-card draws in Solitaire
  and Canfield, Vegas mode in Solitaire, and loading custom layouts in Taipei.

See [ACE_2.0.txt](ACE_2.0.txt) for the complete list.

## Playing

In any game:

|     Key     | Action |
| ----------- | ------ |
| `h` or `F1` |  Help  |
| `q`         |  Quit  |

Each game's Help screen explains its rules and controls.

Your scores are saved in `GameScores.xml`. On Linux it's in your home
directory; on Windows it's in the same folder as the game executables.

## Precompiled Downloads

If you'd rather not build the games yourself, precompiled versions for
**Windows** and the **Steam Deck** are available as zip files on the
[Releases](https://github.com/ChuckBown/Ace_of_Penguins_Solitaire_v2.0/releases)
page. Download the zip for your system from the latest release.

## Building

You will need Qt5 or Qt6 and CMake. From the top-level directory:

```bash
cmake -B build -S .
cmake --build build
```

The compiled games are placed in the `build` directory.

On Windows, copy the executables into a new folder and run `windeployqt` on
that folder to add the Qt libraries they need. The folder can then be moved
to any location or computer.

Platform-specific notes, including Steam Deck setup, are in
[ACE_2.0.txt](ACE_2.0.txt).

## Credits

- **DJ Delorie** created the Ace of Penguins games and libraries.
- **Martin Thornquist** wrote Canfield and Thornq.
- **Eric Bachalo** wrote Spider.
- **Chuck Bown** ported the graphics library from X11 to Qt and added Yukon,
  persistent score keeping and the game launcher.

See [AUTHORS](AUTHORS) for additional contributors.

## License

Distributed under the terms of the GNU General Public License. See
[COPYING](COPYING) for details.
