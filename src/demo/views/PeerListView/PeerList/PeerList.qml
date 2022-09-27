import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/TrayTabRect"
import "qrc:/dc/common/common.js" as Common

ColumnLayout {
    id: layout
    spacing: 5

    property alias model : idList.model

    property string header: ""


    DCText {
        font.bold: true
        text: header
    }

    Rectangle {
        color: Common.darkGray
        Layout.fillHeight: true
        Layout.fillWidth: true
        border.color: Common.borderColor
        border.width: 1
        radius: 2

        ListView {
            id: idList
            anchors.fill: parent

            delegate: Rectangle {

                width: parent.width
                height: 24
                color: Common.transparent

                DCText {
                    leftPadding: 10
                    verticalAlignment : Text.AlignVCenter
                    height: parent.height
                    width: parent.width
                    text: id + " - " + group
                }
            }

        }
    }
}
