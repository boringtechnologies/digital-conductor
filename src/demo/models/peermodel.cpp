//******************************************************************************
//******************************************************************************

#include "peermodel.h"
#include "appcontroller.h"

#include "core/grpcclient.h"
#include "autodiscovery.pb.h"

#include <QTimer>

//******************************************************************************
//******************************************************************************
class PeerModel::impl : public QObject
{
    friend class PeerModel;

protected:
    impl(PeerModel * owner);

    void processUpdate();

protected:
    PeerModel            * m_o;

    QHash<int, QByteArray> m_names;

    dc::PeerType           m_type;
    std::vector<dc::Peer>  m_peers;

    QTimer                 m_updatePeersTimer;
};

//******************************************************************************
//******************************************************************************
PeerModel::impl::impl(PeerModel * owner)
    : m_o(owner)
    , m_type(dc::PeerType::UNKNOWN)
{
    connect(&m_updatePeersTimer, &QTimer::timeout, this, &impl::processUpdate);
}

//******************************************************************************
//******************************************************************************
void PeerModel::impl::processUpdate()
{
    AppController * ac = AppController::instance();
    std::vector<dc::Peer> peers = ac->getPeers();
    m_o->beginResetModel();
    m_peers.clear();
    for (const dc::Peer & p : peers)
    {
        if (p.type() & m_type)
        {
            m_peers.emplace_back(p);
        }
    }
    m_o->endResetModel();
}

//******************************************************************************
//******************************************************************************
PeerModel::PeerModel(QObject * parent)
    : QAbstractListModel(parent)
    , m_p(new impl(this))
{
    m_p->m_names[RoleId]     = "id";
    m_p->m_names[RoleGroup]  = "group";
    m_p->m_names[RoleType]   = "type";

    m_p->processUpdate();
    m_p->m_updatePeersTimer.start(5000);
}

//******************************************************************************
//******************************************************************************
void PeerModel::setType(const QString & type)
{
    if (type == "receiver")
    {
        m_p->m_type = dc::PeerType::READER;
    }
    else if (type == "sender")
    {
        m_p->m_type = dc::PeerType::WRITER;
    }
    else
    {
        m_p->m_type = dc::PeerType::UNKNOWN;
    }
}

//******************************************************************************
//******************************************************************************
int PeerModel::rowCount(const QModelIndex&) const
{
    return m_p->m_peers.size();
}

//******************************************************************************
//******************************************************************************
QVariant PeerModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_p->m_peers.size())
    {
        return QVariant();
    }

    switch (role)
    {
        case RoleId:
            return QString::fromStdString(m_p->m_peers[row].id());
        case RoleGroup:
            return QString::fromStdString(m_p->m_peers[row].group());
        case RoleType:
            return m_p->m_peers[row].type();
    }

    return QVariant();
}

//******************************************************************************
//******************************************************************************
void PeerModel::itemSelected(const quint32 id)
{
    qDebug() << Q_FUNC_INFO << "selected " << id;
    if (id < static_cast<quint32>(m_p->m_peers.size()))
    {
    }
}

//******************************************************************************
//******************************************************************************
QHash<int, QByteArray> PeerModel::roleNames() const
{
    return m_p->m_names;
}
