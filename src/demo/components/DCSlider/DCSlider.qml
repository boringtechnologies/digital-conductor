import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts

import "qrc:/dc/components/DCText"
import "qrc:/dc/common/common.js" as Common

Row {
    property bool isBig: false
    property bool isHeaderBold: false
    property alias label: labelText.label
    property alias from: control.from
    property alias stepSize: control.stepSize
    property alias to: control.to
    property alias value: control.value
    property alias enable: control.enabled

    spacing: 2

    signal valueSelected(int value)

    DCText {
        id: labelText
        y: control.topPadding + control.availableHeight / 2 - height / 2
        leftPadding: 5
    }

    DCText {
        id: valueText
        y: control.topPadding + control.availableHeight / 2 - height / 2
        leftPadding: 5
        width: 60

        label: value + qsTr(" ms.");
    }

    Slider {
        id: control

        background: Rectangle {
            x: control.leftPadding
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: 210
            implicitHeight: 4
            width: control.availableWidth
            height: implicitHeight
            radius: 2
            color: Common.primaryColor

            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                color: enabled ? Common.secondaryColor : Common.lightGray
                radius: 2
            }
        }

        handle: Rectangle {
            x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
            y: parent.height / 2 - height / 2
            width: isBig ? 26 : 22
            height: isBig ? 26 : 22
            radius: width / 2
        }

        onPressedChanged: {
            if (!pressed) {
                valueSelected(value);
            }
        }

        onMoved: {
            valueText.label = value + qsTr(" ms.");
        }
    }
}
