import QtQuick 2.6
import QtQuick.Controls.Material 2.12
import DC as DC
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCText"

Button {
    id: buttonId
    
    property color buttonColor: Common.midGray
    property string buttonText: ""
    property bool isFontBold: false

    contentItem: DCText {
        horizontalAlignment : Text.AlignHCenter
        font.bold: buttonId.isFontBold
        fontSize: 18
        text: buttonId.buttonText
    }

    background: Rectangle {
        color: buttonColor
        opacity: buttonId.pressed ? 0.6 : 1
        radius: 4
    }
}
