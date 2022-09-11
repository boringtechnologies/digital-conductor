import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import DC as DC
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/TrayTabRect"
import "qrc:/dc/views/AppGui/AppGui.js" as Logic
import "qrc:/dc/views/PeerListView"
import "qrc:/dc/views/ReceiverView"
import "qrc:/dc/views/SenderView"

RowLayout {
    spacing: 10

    ColumnLayout {
        spacing: 1

        Rectangle {
            id: tabContainer
            height: 30
            width: 280
            radius: 2
            Layout.leftMargin: 10
            Layout.topMargin: 10

            TabBar {
                id: appTabs
                anchors.fill: parent
                contentHeight: 30

                Repeater {
                    model: Common.userTypes.length

                    TabButton {
                        clip: true
                        verticalPadding: 5
                        horizontalPadding: 20
                        height: parent.height
                        background: TrayTabRect {
                            border.color: appTabs.currentIndex == index ? Common.secondaryColor : Common.darkGray
                        }
                        contentItem: Rectangle {
                            height: 30
                            width: parent.width
                            color: Common.transparent
                            RowLayout {
                                spacing: 5
                                anchors.fill: parent

                                DCText {
                                    Layout.fillWidth: true
                                    label: Common.userTypes[index]
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight

                                    font.bold: appTabs.currentIndex == index
                                }

                                Rectangle {
                                    height: 6
                                    width: 6
                                    radius: 3
                                    color: (index == 0 && senderView.isEnabled) || (index == 1 && receiverView.isEnabled)
                                        ? Common.secondaryColor
                                        : Common.transparent
                                }
                            }
                        }
                    }
                }
            }
        }

        Item {
            Layout.fillHeight: true
            width: 280
            Layout.leftMargin: 10
            Layout.bottomMargin: 10

            Rectangle {
                height: parent.height
                width: parent.width
                color: Common.darkGray
                border.color: Common.borderColor
                border.width: 1
                radius: 2 

                StackLayout {
                    width: parent.width
                    height: parent.height
                    currentIndex: appTabs.currentIndex

                    SenderView {
                        id: senderView
                        anchors.centerIn: parent.center
                        width: parent.width
                    }

                    ReceiverView {
                        id: receiverView
                        width: parent.width
                    }
                } 
            }
        }
    }

    Rectangle {
        width: 1
        Layout.fillHeight: true
        Layout.topMargin: 10
        Layout.bottomMargin: 10

        color: Common.midGray
    }

    PeerListView {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.bottomMargin: 10
        Layout.rightMargin: 10
        Layout.topMargin: 10
    }
}
