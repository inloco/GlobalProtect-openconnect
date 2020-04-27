#! /bin/bash
sudo apt install -y qt5-default libqt5websockets5-dev qtwebengine5-dev
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.4.0.zip \
 -o GlobalProtect-openconnect-1.4.0.zip -L
unzip GlobalProtect-openconnect-1.4.0.zip
cd GlobalProtect-openconnect-1.4.0
qmake-qt5 CONFIG+=release
make
sudo make install
