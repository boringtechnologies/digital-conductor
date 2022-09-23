#include "DeviceInfo/DeviceInfo.hpp"

DeviceInfo::DeviceInfo(QString id, QString label)
{
    setID(id);
    setLabel(label);
}

DeviceInfo DeviceInfo::toQt(DeviceInfo::DeviceInfoStruct info)
{
    DeviceInfo ret;
    ret.info_ = info;
    return ret;
}

DeviceInfo::DeviceInfoStruct DeviceInfo::fromQt(DeviceInfo info)
{
    return info.info_;
}

QString DeviceInfo::getID() const
{
    return QString::fromStdString(info_.id);
}

void DeviceInfo::setID(QString id)
{
    info_.id = id.toStdString();
}

QString DeviceInfo::getLabel() const
{
    return QString::fromStdString(info_.label);
}

void DeviceInfo::setLabel(QString label)
{
    info_.label = label.toStdString();
}
