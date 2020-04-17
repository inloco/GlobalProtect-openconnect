#! /bin/bash
sudo dnf install -y install qt5-qtbase-devel\
 qt5-qtwebsockets-devel qt5-qtwebengine-devel
cd $(mktemp -d)
curl https://github.com/PauloLuna/GlobalProtect-openconnect/archive/v1.2.2.zip\
 -o GlobalProtect-openconnect-1.2.2.zip -L
unzip GlobalProtect-openconnect-1.2.2.zip
cd GlobalProtect-openconnect-1.2.2
qmake-qt5 CONFIG+=release
make
sudo make install