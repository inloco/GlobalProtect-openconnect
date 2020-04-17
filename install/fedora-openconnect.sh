#! /usr/bin/env bash
sudo dnf install -y gettext autoconf automake\
libproxy-devel libxml2-devel libtool pkg-config\
openssl-devel vpnc-script
cd $(mktemp -d)
curl https://gitlab.com/openconnect/openconnect/-/archive/v8.08/openconnect-v8.08.zip\
 -o openconnect-v8.08.zip -L
unzip openconnect-v8.08.zip
cd openconnect-v8.08
./autogen.sh
./configure
make && make check
sudo make install && sudo ldconfig
# Verify
openconnect --version