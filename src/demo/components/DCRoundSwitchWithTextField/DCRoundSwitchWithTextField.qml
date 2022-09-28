import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/common/common.js" as Common

import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/components/DCTextField"


Switch {
    id: switcherId

    property string header

    property bool isChecked: false

    property var selectModel: []
    property alias placeholder: idTextField.placeholderText
    property alias test: idTextField.text
    property int currentSelectIndex: 0

    signal selected(string value)

    checked: isChecked

    indicator: Rectangle {
        implicitWidth: 48
        implicitHeight: 14
        x: parent.width - switcherId.indicator.width
        y: parent.height/2 - height/2
        radius: 7
        color: switcherId.checked ? Common.secondaryColor : Common.midGray

        Rectangle {
            id: indicatorId
            y: parent.height/2 - height/2
            x: switcherId.checked ? parent.width - width : 0
            width: 26
            height: 26
            radius: 13
        }
    }
    contentItem: Row {
        width: parent.width
        DCText {
            id: headerId
            x: 0
            y: parent.height/2 - height/2
            label: header
            font.bold: true
            opacity: enabled ? 1.0 : 0.3
            rightPadding: 8
        }

        DCTextField {
            id: idTextField
            placeholderText: placeholder
            width: parent.width - headerId.width - indicatorId.width - 30
            leftPadding: 5
            topPadding: 5
            bottomPadding: 5
        }
    }
}
