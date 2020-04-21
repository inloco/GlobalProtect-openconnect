#! /bin/bash
sudo dnf groupinstall "Development Tools" -y
sudo dnf install -y qt5-qtbase-devel \
 qt5-qtwebsockets-devel qt5-qtwebengine-devel
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.3.2.zip \
 -o GlobalProtect-openconnect-1.3.2.zip -L
unzip GlobalProtect-openconnect-1.3.2.zip
cd GlobalProtect-openconnect-1.3.2
qmake-qt5 CONFIG+=release
make
sudo make install
