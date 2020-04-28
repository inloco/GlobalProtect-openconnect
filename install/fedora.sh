#! /bin/bash
sudo dnf groupinstall "Development Tools" -y
sudo dnf install -y qt5-qtbase-devel \
 qt5-qtwebsockets-devel qt5-qtwebengine-devel
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.4.2.zip \
 -o GlobalProtect-openconnect-v1.4.2.zip -L
unzip GlobalProtect-openconnect-v1.4.2.zip
cd GlobalProtect-openconnect-v1.4.2
qmake-qt5 CONFIG+=release
make
sudo make install
