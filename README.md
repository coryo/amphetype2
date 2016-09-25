# amphetype2

A rewrite of [Amphetype](https://code.google.com/p/amphetype/) in C++.

using:

*   Qt 5.7
*   QCustomPlot 1.3.1
*   SQLite 3.8.10.2
*   sqlite3pp
*   CMake 3.6.2


## Building

### OSX Build

```bash
git clone https://github.com/coryo/amphetype2.git
cd amphetype2
mkdir build && cd "$_"
cmake .. -DQTROOT=$QT
make
$QT/bin/macdeployqt src/amphetype2.app
```