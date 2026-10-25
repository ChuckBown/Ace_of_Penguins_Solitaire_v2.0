OK, had to use cmake to build.  the makefiles wouldn't work.


need to run the  reinstall_ace_packages   script.
The first command configures everything and the second one actually does the build.

(done from the root ace of penguins directory)
cmake -B build -S . --fresh
cmake --build build




if run into errors MAY need:
export OPENGL_INCLUDE_DIR=/usr/lib
export CMAKE_PREFIX_PATH=/usr/lib/cmake/QT6
export LD_LIBRARY_PATH=/usr/lib


.desktop file goes in ~/.local/share/applications
May want to edit the path depending on where you put the games.
