.import DC as DC

const appTabs = ["Sender", "Receiver"]

function getPeerList() {
    const peerList = DC.PeerController.getPeerList()

    return [
        peerList.filter((el) => el.isSender),
        peerList.filter((el) => el.isReceiver)
    ]
}
