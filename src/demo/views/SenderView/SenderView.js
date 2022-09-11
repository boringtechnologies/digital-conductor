.import DC as DC

function getInputDevices() {
    return DC.DeviceController.getDeviceList(DC.audioDirection.Input)
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

    DC.SenderController.setInputDevice(DC.audioDirection.Input, devId)
}
