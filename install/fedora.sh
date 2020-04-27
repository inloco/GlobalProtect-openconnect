#! /bin/bash
sudo dnf groupinstall "Development Tools" -y
sudo dnf install -y qt5-qtbase-devel \
 qt5-qtwebsockets-devel qt5-qtwebengine-devel
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/stable.zip \
 -o GlobalProtect-openconnect-stable.zip -L
unzip GlobalProtect-openconnect-stable.zip
cd GlobalProtect-openconnect-stable
qmake-qt5 CONFIG+=release
make
sudo make install
