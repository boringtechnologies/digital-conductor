#pragma once

#include "ControllerBase/ControllerBase.hpp"
#include "PeerInfo/PeerInfo.hpp"

#include <QList>

class PeerController : public QObject, public ControllerBase<PeerController>
{
    Q_OBJECT

  public:
    Q_INVOKABLE QList<PeerInfo> getPeerList();

  signals:
    void peerListChanged();

  private slots:
    void onPeerListChanged();

  private:
    friend class ControllerBase;

    PeerController();
    ~PeerController() override;

    void onPeerEvent();
};
