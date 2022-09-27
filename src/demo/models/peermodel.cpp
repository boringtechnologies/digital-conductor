//******************************************************************************
//******************************************************************************

#include "peermodel.h"

//******************************************************************************
//******************************************************************************
PeerModel::PeerModel(QObject *parent)
    : QAbstractListModel{parent}
{
    m_names[RoleId]     = "id";
    m_names[RoleGroup]  = "group";
    m_names[RoleType]   = "type";

}

//******************************************************************************
//******************************************************************************
int PeerModel::rowCount(const QModelIndex&) const
{
    return m_peers.size();
}

//******************************************************************************
//******************************************************************************
QVariant PeerModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_peers.size())
    {
        return QVariant();
    }

    switch (role)
    {
        case RoleId:
            return QString::fromStdString(m_peers[row].id());
        case RoleGroup:
            return QString::fromStdString(m_peers[row].group());
        case RoleType:
            return m_peers[row].type();
    }

    return QVariant();
}

//******************************************************************************
//******************************************************************************
void PeerModel::itemSelected(const quint32 id)
{
    qDebug() << Q_FUNC_INFO << "selected " << id;
    if (id < static_cast<quint32>(m_peers.size()))
    {
    }
}

//******************************************************************************
//******************************************************************************
QHash<int, QByteArray> PeerModel::roleNames() const
{
    return m_names;
}
