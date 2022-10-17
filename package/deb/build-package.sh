#/bin/sh

# sudo apt-get install fakeroot dpkg debconf debhelper lintian 

md5deep -r digitalconductor/usr > digitalconductor/DEBIAN/md5sums
fakeroot dpkg-deb --build digitalconductor
mv digitalconductor.deb ../../bin/digitalconductor_1.0-1_amd64.deb
