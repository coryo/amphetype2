# amphetype2

[![Build Status](https://travis-ci.org/coryo/amphetype2.svg?branch=master)](https://travis-ci.org/coryo/amphetype2)


# Building


## Dependencies:

* Qt>=5.6.1
* CMake>=3.2.3


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
