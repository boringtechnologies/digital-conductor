import QtQuick
import QtQuick.Controls.Universal
import QtQuick.Layouts
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCTextField"

Button {
    property string url: ""
    property string title: ""

    Layout.alignment: Qt.AlignHCenter
    padding: 0
    background: Rectangle {
        color: Common.transparent
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor

        onClicked: {
            Qt.openUrlExternally(url);
        }
    }
    contentItem: DCText{
        text: title
        fontSize: 16
        font.underline: true
    }
}
