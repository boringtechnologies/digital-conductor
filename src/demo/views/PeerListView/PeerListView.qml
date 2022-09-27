import QtQuick 2.6
import QtQuick.Controls.Universal 2.0
import QtQuick.Layouts

import DC.model 1.0

import "qrc:/dc/components/DCRoundSwitch"
import "qrc:/dc/components/DCText"
import "qrc:/dc/components/TrayTabRect"
import "qrc:/dc/views/PeerListView/PeerList"
import "qrc:/dc/common/common.js" as Common
import "qrc:/dc/views/PeerListView/PeerListView.js" as Logic

Column {
    spacing: 10

    Component.onCompleted: () => {}

    PeerModel {
        id: idSenders
        type: "sender"
    }

    PeerModel {
        id: idReceivers
        type: "receiver"
    }

    PeerList {
        width: 160
        height: parent.height / 2 - 5
        header: "Active Senders"
        model: idSenders
    }

    PeerList {
        width: 160
        height: parent.height / 2 - 5
        header: "Active Receivers"
        model: idReceivers
    }
}
