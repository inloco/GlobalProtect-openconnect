#! /bin/bash
sudo apt install -y qt5-default libqt5websockets5-dev qtwebengine5-dev
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.4.2.zip \
 -o GlobalProtect-openconnect-v1.4.2.zip -L
unzip GlobalProtect-openconnect-v1.4.2.zip
cd GlobalProtect-openconnect-v1.4.2
qmake CONFIG+=release
make
sudo make install
