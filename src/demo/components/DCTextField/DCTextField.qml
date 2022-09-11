import QtQuick
import QtQuick.Controls.Universal
import "qrc:/dc/common/common.js" as Common

TextField {
    id: textFieldId
    
    property int fontSize: 16
    property real globalScaleFactor: Screen.pixelDensity * 10 * 2.54 / 160
    readonly property color appColor: Common.secondaryColor

    color: Common.textColor
    font.pixelSize: fontSize * globalScaleFactor
    bottomPadding: 10
    topPadding: 10
    selectionColor: Common.secondaryColor
    placeholderTextColor: Common.lightGray

    background: Rectangle {
        color: Common.midGray
        radius: 4
        border.width: 1
        border.color: textFieldId.activeFocus ? textFieldId.appColor : Common.borderColor
    }
}
