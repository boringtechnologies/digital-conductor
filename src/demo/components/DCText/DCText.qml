import QtQuick 2.6
import QtQuick.Window 2.2
import QtQuick.Controls.Universal 2.0
import "qrc:/dc/common/common.js" as Common

Text {
    property string label: ""
    property int fontSize: 16
    property real globalScaleFactor: Screen.pixelDensity * 10 * 2.54 / 160

    text: label
    font.pixelSize: fontSize * globalScaleFactor
    topPadding: 0
    bottomPadding: 0
    color: Common.textColor
}
