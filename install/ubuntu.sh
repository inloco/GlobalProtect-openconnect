#! /bin/bash
sudo apt install -y qt5-default libqt5websockets5-dev qtwebengine5-dev vpnc resolvconf
cd $(mktemp -d)
curl https://github.com/inloco/GlobalProtect-openconnect/archive/v1.4.3.zip \
 -o GlobalProtect-openconnect-v1.4.3.zip -L
unzip GlobalProtect-openconnect-v1.4.3.zip
cd GlobalProtect-openconnect-1.4.3
qmake CONFIG+=release
make
sudo make install
sudo systemctl daemon-reload
sudo systemctl stop gpservice.service
