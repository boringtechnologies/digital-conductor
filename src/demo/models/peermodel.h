//******************************************************************************
//******************************************************************************

#ifndef PEERMODEL_H
#define PEERMODEL_H

#include <QAbstractListModel>

#include <memory>
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
    Q_PROPERTY(QString type WRITE setType)

  public:
    explicit PeerModel(QObject *parent = nullptr);

  public:
    void setType(const QString &type);

  public:
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

  public slots:
    virtual void selectItem(const quint32 id);

  signals:

  private:
    QHash<int, QByteArray> roleNames() const;

  private:
    class impl;
    std::shared_ptr<impl> m_p;
};

#endif // PEERMODEL_H
