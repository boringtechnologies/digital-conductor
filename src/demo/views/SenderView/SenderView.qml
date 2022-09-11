import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/views/SenderView/SenderView.js" as Logic
import "qrc:/dc/common/common.js" as Common

Rectangle {
    id: senderView
    property var inputDevices: Logic.getInputDevices()
    property bool isEnabled: false
    
    color: Common.transparent


    Component.onCompleted: () => {
        inputDeviceSelector.onCurrentIndexChanged.connect(() => {
            Logic.setInputDevice()
        })
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 20
        anchors.rightMargin: 10
        spacing: 20

        DCRoundSwitch {
            id: senderSwitch
            Layout.fillWidth: true
            header: "Start Sending"
            isHeaderBold: true
            isBig: true
            onClicked: Logic.setSenderState()
        }

        Rectangle {
            height: 1
            width: parent.width
            color: Common.midGray
        }

        Column {
            Layout.fillWidth: true
            DCText {
                label: "Audio Input Device"
                bottomPadding: 5
            }
            DCComboBox {
                id: inputDeviceSelector
                width: parent.width
                model: inputDevices.map((el) => el.label)
            }
        }
    }
}
