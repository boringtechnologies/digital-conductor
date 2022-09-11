#include "PeerController/PeerController.hpp"

PeerController::PeerController()
{
}

PeerController::~PeerController()
{
}

QList<PeerInfo> PeerController::getPeerList()
{
    QList<PeerInfo> result;

    result.push_back(PeerInfo("sender1", true, false));
    result.push_back(PeerInfo("sender2", true, false));
    result.push_back(PeerInfo("sender3", true, false));
    result.push_back(PeerInfo("receiver1", false, true));
    result.push_back(PeerInfo("receiver2", false, true));
    result.push_back(PeerInfo("sender-receiver1", true, true));
    result.push_back(PeerInfo("sender-receiver2", true, true));

    return result;
}

void PeerController::onPeerListChanged()
{
    peerListChanged();
}

void PeerController::onPeerEvent()
{
    QMetaObject::invokeMethod(this, "onPeerListChanged", Qt::QueuedConnection);
}
