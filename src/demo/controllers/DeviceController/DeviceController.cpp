#include "DeviceController/DeviceController.hpp"

DeviceController::DeviceController()
{
}

DeviceController::~DeviceController()
{
}

QList<AudioDeviceInfo> DeviceController::getDeviceList(AudioDirectionValue dir)
{
    QList<AudioDeviceInfo> result;

    if (dir ==  AudioDirectionValue::Input) {
        result.push_back(AudioDeviceInfo("inputDeviceId1", "Headphones Microphone"));
        result.push_back(AudioDeviceInfo("inputDeviceId2", "MacBook Air Microphone"));
    } else {
        result.push_back(AudioDeviceInfo("OutputDeviceId1", "Headphone"));
        result.push_back(AudioDeviceInfo("OutputDeviceId2", "Air Pods"));
    }

    return result;
}

void DeviceController::onDeviceListChanged()
{
    deviceListChanged();
}
