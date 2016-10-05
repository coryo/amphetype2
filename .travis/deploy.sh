#!/bin/bash

if [[ $TRAVIS_OS_NAME == 'osx' ]]; then
  ${QTDIR}/bin/macdeployqt src/amphetype2.app
  mv src/amphetype2.app .
  tar -czf amphetype2-osx.tar.gz amphetype2.app txt/
  export DEPLOYFILE=amphetype2-osx.tar.gz
else
  mkdir dist
  cp bundle/linux/* dist/
  mv src/amphetype2 dist/
  sudo wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/1/linuxdeployqt-1-x86_64.AppImage" -O /usr/local/bin/linuxdeployqt
  sudo chmod a+x /usr/local/bin/linuxdeployqt
  linuxdeployqt dist/amphetype2 -appimage -bundle-non-qt-libs -verbose=2
  mv dist.AppImage amphetype2.AppImage
  tar -czf amphetype2-linux-x86_64-AppImage.tar.gz amphetype2.AppImage txt/
  export DEPLOYFILE=amphetype2-linux-x86_64-AppImage.tar.gz
fi