//******************************************************************************
//******************************************************************************
#ifndef DEVICEMODEL_H
#define DEVICEMODEL_H

#include <QAbstractListModel>

#include <memory>

//******************************************************************************
//******************************************************************************
class DeviceModel : public QAbstractListModel
{
    Q_OBJECT

    enum Role
    {
        RoleId = Qt::UserRole + 1,
        RoleName
    };

public:
    DeviceModel(QObject *parent = nullptr);

public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

public slots:
    virtual void itemSelected(const quint32 id);

signals:

private:
    QHash<int, QByteArray> roleNames() const;

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

#endif // DEVICEMODEL_H
