import QtQuick 2.6
import QtQuick.Controls.Material 2.12
import QtQuick.Layouts
import DC as DC
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"

ColumnLayout {
    anchors.leftMargin: 20
    anchors.topMargin: 20
    anchors.rightMargin: 20

    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    spacing: 20

    DCText {
        Layout.alignment: Qt.AlignHCenter
        text: "Please Wait!"
        fontSize: 20
        font.bold: true
        wrapMode: "WordWrap"
        elide: Text.ElideRight
    }
        
    DCText {
        Layout.alignment: Qt.AlignHCenter
        text: "We are checking your license"
        fontSize: 18
        wrapMode: "WordWrap"
        elide: Text.ElideRight
    }

    Rectangle {
        height: 150
        width: 150
        color: Common.transparent
        Layout.alignment: Qt.AlignHCenter

        BusyIndicator {
            height: parent.height
            width: parent.width
            Material.accent: Common.secondaryColor
        }
    }
}

