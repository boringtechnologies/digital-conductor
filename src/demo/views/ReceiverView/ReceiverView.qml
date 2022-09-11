import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/views/ReceiverView/ReceiverView.js" as Logic
import "qrc:/dc/common/common.js" as Common

Rectangle {
    id: receiverView
    property var outputDevices: Logic.getOutputDevices()
    property bool isEnabled: false

    color: Common.transparent

    Component.onCompleted: () => {
        outputDeviceSelector.onCurrentIndexChanged.connect(() => {
            Logic.setOutputDevice()
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
            id: receiverSwitch
            Layout.fillWidth: true
            header: "Start Receiving"
            isBig: true
            isHeaderBold: true
            onClicked: Logic.setReceiverState()
        }

        Rectangle {
            height: 1
            width: parent.width
            color: Common.midGray
        }

        Column {
            Layout.fillWidth: true
            DCText {
                label: "Audio Output Device"
                bottomPadding: 5
            }
            DCComboBox {
                id: outputDeviceSelector
                width: parent.width
                model: outputDevices.map((el) => el.label)
            }
        }

        Rectangle {
            height: 1
            width: parent.width
            color: Common.midGray
        }

        DCRoundSwitch {
            id: syncSwitch
            Layout.fillWidth: true
            header: "Synchronization"
            onClicked: Logic.setSyncMode()
        }
    }
}
