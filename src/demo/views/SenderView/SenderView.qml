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
    id: senderView

    property bool isEnabled: false
    
    color: Common.transparent

    Component.onCompleted: {
        Controller.onSenderViewReady()
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
            id: senderSwitch
            Layout.fillWidth: true
            header: qsTr("Start Sending to")
            disabledEditTooltip: qsTr("Stop streaming and recieving to change group")
            textValue: Controller.group
            onClicked: {
                checked ? Controller.startStreaming() : Controller.stopStream();
            }

            onGroupChanged: newGroup => Controller.groupChanged(newGroup);

            Connections {
                target: Controller

                function onSetSenderBusy(busy) {
                    senderSwitch.isBusy = busy;
                    senderSwitch.enabled = !busy;
                }

                function onSetGroupEditable(editable) {
                    senderSwitch.isEditable = editable;
                }

                function onUpdatedGroup(group) {
                    senderSwitch.textValue = group;
                }
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
                label: "Audio Input Device"
                bottomPadding: 5
            }

            DeviceModel {
                id: idAudioInDevices
                deviceClass: "Audio/Source"
            }

            DCComboBox {
                id: inputDeviceSelector
                width: parent.width
                model: idAudioInDevices
                currentIndex: idAudioInDevices.currentIndex

                onCurrentIndexChanged: {
                    idAudioInDevices.selectItem(currentIndex);
                }

                onRequestUpdate: {
                    idAudioInDevices.update()
                }
            }
            Component.onCompleted: {
                idAudioInDevices.update();
            }
        }

        Column {
            Layout.fillWidth: true
            DCText {
                label: "Video Input Device"
                bottomPadding: 5
            }
            DeviceModel {
                id: idVideoInDevices
                deviceClass: "Video/Source"
            }
            DCComboBox {
                id: inputVideoDeviceSelector
                width: parent.width
                model: idVideoInDevices
                currentIndex: idVideoInDevices.currentIndex

                onCurrentIndexChanged: {
                    idVideoInDevices.selectItem(currentIndex);
                }

                onRequestUpdate: {
                    idVideoInDevices.update()
                }
            }
            Component.onCompleted: {
                idVideoInDevices.update();
            }
        }
    }
}
