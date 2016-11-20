#!/bin/bash
set -ex

PACKAGES="make
automake
autoconf
autotools-dev
libopts25-dev
libncurses5-dev
libtool
libssl-dev"

# apt-get update
# apt-get upgrade
apt-get install $PACKAGES
# apt-get autoremove
# apt-get autoclean
./bootstrap
./configure
make
make install
chown -R $SUDO_USER:$SUDO_USER . ~/.motoko
