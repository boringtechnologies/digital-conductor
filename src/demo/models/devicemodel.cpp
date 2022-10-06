//******************************************************************************
//******************************************************************************

#include "devicemodel.h"
#include "core/media/util.h"

#include <vector>
#include <string>

//******************************************************************************
//******************************************************************************
class DeviceModel::impl : public QObject
{
    friend class DeviceModel;

protected:
    impl(DeviceModel * owner);

    void processUpdate();

protected:
    DeviceModel            * m_o;

    QHash<int, QByteArray>   m_names;

    std::vector<std::string> m_devices;
};

//******************************************************************************
//******************************************************************************
DeviceModel::impl::impl(DeviceModel * owner)
    : m_o(owner)
{
}

//******************************************************************************
//******************************************************************************
DeviceModel::DeviceModel(QObject * parent)
    : QAbstractListModel(parent)
    , m_p(new impl(this))
{
    m_p->m_names[RoleId]     = "id";
    m_p->m_names[RoleName]   = "name";

    dc::media::Util util = dc::media::Util::instance();
    m_p->m_devices = util.getDeviceList();
}

//******************************************************************************
//******************************************************************************
int DeviceModel::rowCount(const QModelIndex&) const
{
    return m_p->m_devices.size();
}

//******************************************************************************
//******************************************************************************
QVariant DeviceModel::data(const QModelIndex &index, int role) const
{
    int row = index.row();
    if (row < 0 || row >= m_p->m_devices.size())
    {
        return QVariant();
    }

    switch (role)
    {
        case RoleId:
            return row;
        case RoleName:
            return QString::fromStdString(m_p->m_devices[row]);
    }

    return QVariant();
}

//******************************************************************************
//******************************************************************************
void DeviceModel::itemSelected(const quint32 id)
{
    qDebug() << Q_FUNC_INFO << "selected " << id;
    if (id < static_cast<quint32>(m_p->m_devices.size()))
    {
    }
}

//******************************************************************************
//******************************************************************************
QHash<int, QByteArray> DeviceModel::roleNames() const
{
    return m_p->m_names;
}
