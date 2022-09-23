#pragma once

#include "DeviceInfo/DeviceInfo.hpp"
#include "DeviceDirection/DeviceDirection.hpp"
#include "ControllerBase/ControllerBase.hpp"

#include <QList>

class AudioDeviceController : public QObject,
                         public ControllerBase<AudioDeviceController>
{
    Q_OBJECT

public:
    Q_INVOKABLE QList<DeviceInfo> getDeviceList(DeviceDirectionValue dir);

signals:
    void deviceListChanged();

private slots:
    void onDeviceListChanged();

private:
    friend class ControllerBase;

    AudioDeviceController();
    ~AudioDeviceController() override;
};
