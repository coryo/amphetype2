# amphetype2 [![Build Status](https://travis-ci.org/coryo/amphetype2.svg?branch=master)](https://travis-ci.org/coryo/amphetype2)

A cross-platform program to help you practice typing. based on [amphetype](https://code.google.com/archive/p/amphetype/).

<a href="https://cloud.githubusercontent.com/assets/678207/19257423/86538294-8f3d-11e6-9e2e-e63e645a9168.PNG">
<img src="https://cloud.githubusercontent.com/assets/678207/19257423/86538294-8f3d-11e6-9e2e-e63e645a9168.PNG" width="420"></a>
<a href="https://cloud.githubusercontent.com/assets/678207/19257568/7fe3f942-8f3e-11e6-8690-2ff04d313f4b.PNG">
<img src="https://cloud.githubusercontent.com/assets/678207/19257568/7fe3f942-8f3e-11e6-8690-2ff04d313f4b.PNG" width="420"></a>


# Building


## Dependencies:

* Qt>=5.6
* CMake>=3.2.2


## Windows

See `build.bat`.


## Linux / OSX

```bash
git clone https://github.com/coryo/amphetype2.git
cd amphetype2
cmake . -DQTROOT=$QT
make
```
