#/bin/sh

BIN_PATH=../../bin/

cqtdeployer -confFile ./CQtDeployer.json -qmake ${QT_ROOT}/bin/qmake -verbose 3

cp DistributionKit/*.deb* ${BIN_PATH}
rm -rf DistributionKit

dpkg -I ${BIN_PATH}/*.deb
