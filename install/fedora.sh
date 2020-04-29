#! /bin/bash
sudo dnf groupinstall "Development Tools" -y
sudo dnf install -y qt5-qtbase-devel \
 qt5-qtwebsockets-devel qt5-qtwebengine-devel
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.4.3.zip \
 -o GlobalProtect-openconnect-v1.4.3.zip -L
unzip GlobalProtect-openconnect-v1.4.3.zip
cd GlobalProtect-openconnect-1.4.3
qmake-qt5 CONFIG+=release
make
sudo make install
