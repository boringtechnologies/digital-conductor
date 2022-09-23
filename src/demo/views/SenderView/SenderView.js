.import DC as DC

function getAudioInputDevices() {
    return DC.AudioDeviceController.getDeviceList(DC.deviceDirection.Input)
}

function getVideoInputDevices() {
    return DC.VideoDeviceController.getDeviceList(DC.deviceDirection.Input)
}

function setSenderState() {
    senderView.isEnabled = senderSwitch.checked
    
    DC.SenderController.setState(senderSwitch.checked
        ? DC.SenderController.State.Running
        : DC.SenderController.State.Stopped)
}

function setInputDevice() {
    const index = inputDeviceSelector.currentIndex

    if (index == -1) {
        return
    }

    const devId = senderView.inputDevices[index].id

    DC.SenderController.setInputDevice(DC.deviceDirection.Input, devId)
}
