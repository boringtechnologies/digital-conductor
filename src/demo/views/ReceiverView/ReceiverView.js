.import DC as DC

function getAudioOutputDevices() {
    return DC.AudioDeviceController.getDeviceList(DC.deviceDirection.Output)
}

function getVideoOutputDevices() {
    return DC.VideoDeviceController.getDeviceList(DC.deviceDirection.Output)
}

function setReceiverState() {
    receiverView.isEnabled = receiverSwitch.checked
    
    DC.ReceiverController.setState(receiverSwitch.checked
        ? DC.ReceiverController.State.Running
        : DC.ReceiverController.State.Stopped)
}

function setSyncMode() {
    DC.ReceiverController.setSyncMode(syncSwitch.checked
        ? DC.ReceiverController.SyncMode.On
        : DC.ReceiverController.SyncMode.Off)
}

function setOutputDevice() {
    const index = outputDeviceSelector.currentIndex

    if (index == -1) {
        return
    }

    const devId = receiverView.outputDevices[index].id

    DC.ReceiverController.setOutputDevice(DC.deviceDirection.Output, devId)
}
