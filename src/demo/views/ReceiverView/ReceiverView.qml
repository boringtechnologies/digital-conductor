import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts

import DC.controller 1.0
import DC.model 1.0

import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/components/DCRoundSwitchWithTextField"
import "qrc:/dc/views/ReceiverView/ReceiverView.js" as Logic
import "qrc:/dc/common/common.js" as Common

Rectangle {
    id: receiverView
    property var audioOutputDevices: Logic.getAudioOutputDevices()
    property var videoOutputDevices: Logic.getVideoOutputDevices()

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

        DCRoundSwitchWithTextField {
            id: receiverSwitch
            Layout.fillWidth: true
            header: qsTr("Start Receiving from")
            placeholder: Controller.group
            // onClicked: Logic.setReceiverState()
            onClicked: {
                checked ? Controller.receiveStream() : Controller.stopStream();
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
                    console.log(currentIndex);
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

            DeviceModel {
                id: idVideoOutDevices
                // deviceClass: "Video/Source"
            }

            DCComboBox {
                id: outputVideoDeviceSelector
                width: parent.width
                model: idVideoOutDevices
                currentIndex: idVideoOutDevices.currentIndex

                onCurrentIndexChanged: {
                    console.log(currentIndex);
                }

                onRequestUpdate: {
                    idVideoOutDevices.update()
                }
            }

            Component.onCompleted: {
                idVideoOutDevices.update();
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
