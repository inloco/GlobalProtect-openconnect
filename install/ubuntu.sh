#! /bin/bash
sudo apt install -y qt5-default libqt5websockets5-dev qtwebengine5-dev
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/stable.zip \
 -o GlobalProtect-openconnect-stable.zip -L
unzip GlobalProtect-openconnect-stable.zip
cd GlobalProtect-openconnect-stable
qmake CONFIG+=release
make
sudo make install
