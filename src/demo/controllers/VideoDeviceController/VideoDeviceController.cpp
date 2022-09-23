#include "VideoDeviceController/VideoDeviceController.hpp"

VideoDeviceController::VideoDeviceController()
{
}

VideoDeviceController::~VideoDeviceController()
{
}

QList<DeviceInfo> VideoDeviceController::getDeviceList(DeviceDirectionValue dir)
{
    QList<DeviceInfo> result;

    if (dir ==  DeviceDirectionValue::Input) {
        result.push_back(DeviceInfo("inputDeviceId1", "Input Device 1"));
        result.push_back(DeviceInfo("inputDeviceId2", "Input Device 2"));
    } else {
        result.push_back(DeviceInfo("OutputDeviceId1", "Output Device 1"));
        result.push_back(DeviceInfo("OutputDeviceId2", "Output Device 2"));
    }

    return result;
}

void VideoDeviceController::onDeviceListChanged()
{
    deviceListChanged();
}
