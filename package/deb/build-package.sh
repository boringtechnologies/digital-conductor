#/bin/sh

PROJ_ROOT="$(pwd)/../../"

cqtdeployer -deb $(pwd)/digitalconductor/DEBIAN         \
            -bin ${PROJ_ROOT}/bin/digitalconductor-demo \
            -qmake ${QT_ROOT}/bin/qmake -binOut bin     \
            -qmlDir ${PROJ_ROOT}/src/demo/              \
            -debOut digitalconductor_1.0-1_amd64

mv DistributionKit/digitalconductor* ../../bin/
