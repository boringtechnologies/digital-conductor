import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts

import DC.controller 1.0
import DC.model 1.0

import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/components/DCRoundSwitchWithTextField"
import "qrc:/dc/common/common.js" as Common

Rectangle {
    id: receiverView

    property bool isEnabled: false

    color: Common.transparent

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.leftMargin: 10
        anchors.topMargin: 20
        anchors.rightMargin: 10
        spacing: 20

        DCRoundSwitchWithTextField {
            id: receiverSwitch
            Layout.fillWidth: true
            header: qsTr("Start Receiving from")
            placeholder: Controller.group
            onClicked: {
                checked ? Controller.receiveStream() : Controller.stopReceive();
            }
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

            DeviceModel {
                id: idAudioOutDevices
                deviceClass: "Audio/Sink"
            }

            DCComboBox {
                id: outputDeviceSelector
                width: parent.width
                model: idAudioOutDevices
                currentIndex: idAudioOutDevices.currentIndex

                onCurrentIndexChanged: {
                    idAudioOutDevices.selectItem(currentIndex);
                }

                onRequestUpdate: {
                    idAudioOutDevices.update()
                }
            }

            Component.onCompleted: {
                idAudioOutDevices.update();
            }
        }

        Column {
            Layout.fillWidth: true
            DCText {
                label: "Video Output Device"
                bottomPadding: 5
            }

            DCComboBox {
                id: outputVideoDeviceSelector
                enabled: false
                width: parent.width
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
            checked: Controller.syncModeEnabled
            onClicked: {
                Controller.syncModeEnabled = checked;
            }

        }
    }
}
