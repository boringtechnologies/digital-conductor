import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts
import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/TrayTabRect"
import "qrc:/dc/views/PeerListView/PeerList"
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/views/PeerListView/PeerListView.js" as Logic

Column {
    spacing: 10
    property var peers: Logic.getPeerList()

    Component.onCompleted: () => {}

    PeerList {
        width: 160
        height: parent.height / 2 - 5
        header: "Active Senders"
        peerList: peers[0]
    }

    PeerList {
        width: 160
        height: parent.height / 2 - 5
        header: "Active Receivers"
        peerList: peers[1]
    }
}
