#include "AudioDeviceController/AudioDeviceController.hpp"

AudioDeviceController::AudioDeviceController()
{
}

AudioDeviceController::~AudioDeviceController()
{
}

QList<DeviceInfo> AudioDeviceController::getDeviceList(DeviceDirectionValue dir)
{
    QList<DeviceInfo> result;

    if (dir ==  DeviceDirectionValue::Input) {
        result.push_back(DeviceInfo("inputDeviceId1", "Headphones Microphone"));
        result.push_back(DeviceInfo("inputDeviceId2", "MacBook Air Microphone"));
    } else {
        result.push_back(DeviceInfo("OutputDeviceId1", "Headphone"));
        result.push_back(DeviceInfo("OutputDeviceId2", "Air Pods"));
    }

    return result;
}

void AudioDeviceController::onDeviceListChanged()
{
    deviceListChanged();
}
