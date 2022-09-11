#pragma once

#include "AudioDeviceInfo/AudioDeviceInfo.hpp"
#include "AudioDirection/AudioDirection.hpp"
#include "ControllerBase/ControllerBase.hpp"

#include <QList>

class DeviceController : public QObject,
                         public ControllerBase<DeviceController>
{
    Q_OBJECT

public:
    Q_INVOKABLE QList<AudioDeviceInfo> getDeviceList(AudioDirectionValue dir);

signals:
    void deviceListChanged();

private slots:
    void onDeviceListChanged();

private:
    friend class ControllerBase;

    DeviceController();
    ~DeviceController() override;
};
