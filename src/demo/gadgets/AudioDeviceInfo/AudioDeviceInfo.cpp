#include "AudioDeviceInfo/AudioDeviceInfo.hpp"

AudioDeviceInfo::AudioDeviceInfo(QString id, QString label)
{
    setID(id);
    setLabel(label);
}

AudioDeviceInfo AudioDeviceInfo::toQt(AudioDeviceInfo::DeviceInfoStruct info)
{
    AudioDeviceInfo ret;
    ret.info_ = info;
    return ret;
}

AudioDeviceInfo::DeviceInfoStruct AudioDeviceInfo::fromQt(AudioDeviceInfo info)
{
    return info.info_;
}

QString AudioDeviceInfo::getID() const
{
    return QString::fromStdString(info_.id);
}

void AudioDeviceInfo::setID(QString id)
{
    info_.id = id.toStdString();
}

QString AudioDeviceInfo::getLabel() const
{
    return QString::fromStdString(info_.label);
}

void AudioDeviceInfo::setLabel(QString label)
{
    info_.label = label.toStdString();
}
