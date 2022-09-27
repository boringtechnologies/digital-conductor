//******************************************************************************
//******************************************************************************

#ifndef PEERMODEL_H
#define PEERMODEL_H

#include "autodiscovery.grpc.pb.h"

#include <QAbstractListModel>

#include <vector>

//******************************************************************************
//******************************************************************************
class PeerModel : public QAbstractListModel
{
    Q_OBJECT

    enum Role
    {
        RoleId = Qt::UserRole + 1,
        RoleGroup,
        RoleType
    };

public:
    explicit PeerModel(QObject *parent = nullptr);

public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

public slots:
    virtual void itemSelected(const quint32 id);

signals:

private:
    QHash<int, QByteArray> roleNames() const;

private:
    QHash<int, QByteArray> m_names;

    std::vector<dc::Peer>  m_peers;
};

#endif // PEERMODEL_H
