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

    property var peerList: []
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

        ScrollView {
            height: parent.height
            width: parent.width
            contentWidth: parent.width
            ScrollBar.horizontal.policy: ScrollBar.AsNeeded
            ScrollBar.vertical.policy: ScrollBar.AsNeeded

            ColumnLayout {
                width: parent.width
                spacing: 0

                Repeater {
                    model: peerList.length

                    Rectangle {
                        width: parent.width
                        height: 24
                        color: Common.transparent

                        DCText {
                            leftPadding: 10
                            verticalAlignment : Text.AlignVCenter
                            height: parent.height
                            width: parent.width
                            text: peerList[index].id
                        }
                    }
                }
            }
        }
    }
}
