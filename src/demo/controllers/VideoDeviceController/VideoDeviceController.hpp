#pragma once

#include "DeviceInfo/DeviceInfo.hpp"
#include "DeviceDirection/DeviceDirection.hpp"
#include "ControllerBase/ControllerBase.hpp"

#include <QList>

class VideoDeviceController : public QObject,
                         public ControllerBase<VideoDeviceController>
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

    VideoDeviceController();
    ~VideoDeviceController() override;
};
