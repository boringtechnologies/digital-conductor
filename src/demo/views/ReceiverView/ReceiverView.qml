import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts

import DC.controller 1.0
import DC.model 1.0

import "qrc:/dc/components/DCText"
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCComboBox"
import "qrc:/dc/components/DCRoundSwitchWithTextField"
import "qrc:/dc/components/DCSlider"
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
            disabledEditTooltip: qsTr("Stop streaming and recieving to change group")
            textValue: Controller.group
            onClicked: {
                checked ? Controller.receiveStream() : Controller.stopReceive();
            }

            onGroupChanged: newGroup => Controller.groupChanged(newGroup);

            Connections {
                target: Controller

                function onSetReceiverBusy(busy) {
                    receiverSwitch.isBusy = busy;
                    receiverSwitch.enabled = !busy;
                }

                function onSetGroupEditable(editable) {
                    receiverSwitch.isEditable = editable;
                }

                function onUpdatedGroup(group) {
                    receiverSwitch.textValue = group;
                }

                function onMinSyncDelayChanged(minSyncDelay) {
                    if (syncLatency.value < minSyncDelay) {
                        syncLatency.value = minSyncDelay
                    }
                    syncLatency.from = minSyncDelay;
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

        Column {
            Layout.fillWidth: true
            spacing: 15

            DCRoundSwitch {
                id: syncSwitch
                Layout.fillWidth: true
                header: "Synchronization"
                checked: Controller.syncModeEnabled
                onClicked: {
                    Controller.syncModeEnabled = checked;
                    syncLatency.enabled = checked;
                }
            }

            DCSlider {
                id: syncLatency
                Layout.fillWidth: true
                from: 50
                value: Controller.syncDelay;
                to: 2000
                stepSize: 50
                label: "Delay:"

                onValueSelected: value => Controller.setSyncDelay(value)
            }
        }
    }
}
