# amphetype2

# Building

## Dependencies:

* Qt 5.7
* CMake 3.6.2


## Windows

See `build.bat`.

## OSX

`$QT = /Users/me/Qt/5.7/clang_64`

```bash
git clone https://github.com/coryo/amphetype2.git
cd amphetype2
mkdir build && cd build
cmake .. -DQTROOT=$QT
make
$QT/bin/macdeployqt src/amphetype2.app
```
