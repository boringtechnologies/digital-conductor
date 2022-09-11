// App colors
const primaryColor = "#282633"
const secondaryColor = "#1D84B5"
const darkGray = "#43414F"
const midGray = "#565365"
const lightGray = "#D0CFCF"
const borderColor = "#4B485B"
const textColor = "#FFFBFE"
const transparent = "transparent"

const userTypes = ["Sender", "Receiver"]

function setInterval(timer, isRepeat, delayTimeMs, cb) {
    timer.interval = delayTimeMs
    timer.repeat = isRepeat
    timer.triggered.connect(cb)
    timer.start()
}
