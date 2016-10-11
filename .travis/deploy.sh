#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  curl -L -O https://github.com/MaximAlien/macdeployqt/raw/master/macdeployqt.dmg
  hdiutil mount macdeployqt.dmg
  sudo /Volumes/*macdeployqt*/macdeployqt.app/Contents/MacOS/macdeployqt src/amphetype2.app -dmg
  mv src/amphetype2.dmg .
  # tar -czf amphetype2-osx.tar.gz amphetype2.app txt/
  export DEPLOYFILE=amphetype2.dmg
else
  mkdir dist
  cp bundle/linux/* dist/
  mv src/amphetype2 dist/
  wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/1/linuxdeployqt-1-x86_64.AppImage" -O linuxdeployqt.AppImage
  chmod a+x linuxdeployqt.AppImage
  ./linuxdeployqt.AppImage dist/amphetype2 -appimage -bundle-non-qt-libs -verbose=2
  mv dist.AppImage amphetype2-linux-x86_64.AppImage
  # tar -czf amphetype2-linux-x86_64-AppImage.tar.gz amphetype2.AppImage txt/
  export DEPLOYFILE=amphetype2-linux-x86_64.AppImage
fi
