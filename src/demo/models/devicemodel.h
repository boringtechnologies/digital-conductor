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
    Q_PROPERTY(uint32_t currentIndex READ currentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString deviceClass READ deviceClass WRITE setDeviceClass)

public:
    DeviceModel(QObject *parent = nullptr);

    void setDeviceClass(const QString & deviceClass);

public:
    virtual int rowCount(const QModelIndex & parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex & index, int role = Qt::DisplayRole) const;

    uint32_t currentIndex() const;
    QString deviceClass() const;

public slots:
    virtual void update();
    virtual void selectItem(const quint32 id);

signals:
    void currentIndexChanged();

private:
    QHash<int, QByteArray> roleNames() const;

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

#endif // DEVICEMODEL_H
