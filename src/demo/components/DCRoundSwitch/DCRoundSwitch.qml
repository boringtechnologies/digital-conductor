import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/components/DCText"
import "qrc:/dc/common/common.js" as Common


Switch {
    property string header
    property bool isChecked: false
    property bool isWaiting: false
    property bool isBig: false
    property bool isHeaderBold: false

    checked: isChecked
    id: switcherId

    indicator: Rectangle {
        implicitWidth: isBig ? 48 : 40
        implicitHeight: isBig ? 14 : 12
        x: parent.width - switcherId.indicator.width
        y: parent.height/2 - height/2
        radius: 7
        color: switcherId.checked ? Common.secondaryColor : Common.midGray

        Rectangle {
            id: indicatorId
            y: parent.height/2 - height/2
            x: switcherId.checked ? parent.width - width : 0
            width: isBig ? 26 : 22
            height: isBig ? 26 : 22
            radius: width / 2

            BusyIndicator {
                width: isBig ? 24 : 20
                height: isBig ? 24 : 20
                anchors.centerIn: parent
                running: isWaiting
                Universal.accent: Common.secondaryColor
            }
        }
    }
    contentItem: DCText {
        x: 0
        y: parent.height/2 - height/2
        label: header
        font.bold: isHeaderBold
        opacity: enabled ? 1.0 : 0.3
        rightPadding: switcherId.indicator.width + switcherId.spacing
    }
}
