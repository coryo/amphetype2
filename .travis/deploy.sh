#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  sudo chmod +x ./tools/macdeployqt.app/Contents/MacOS/macdeployqt
  sudo ./tools/macdeployqt.app/Contents/MacOS/macdeployqt src/amphetype2.app -dmg
  user=$(id -nu)
  sudo chown ${user} src/amphetype2.dmg
  mv src/amphetype2.dmg .
  export DEPLOYFILE=amphetype2.dmg
else
  mkdir dist
  cp bundle/linux/* dist/
  mv src/amphetype2 dist/
  chmod a+x tools/linuxdeployqt.AppImage
  ./tools/linuxdeployqt.AppImage dist/amphetype2 -appimage -bundle-non-qt-libs -verbose=2
  mv dist.AppImage amphetype2-linux-x86_64.AppImage
  export DEPLOYFILE=amphetype2-linux-x86_64.AppImage
fi
