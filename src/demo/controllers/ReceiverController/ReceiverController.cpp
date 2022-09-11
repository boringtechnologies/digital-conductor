#include "ReceiverController/ReceiverController.hpp"

#include <QDebug>

ReceiverController::ReceiverController()
{
}

ReceiverController::~ReceiverController()
{
}

void ReceiverController::setState(State state) const
{
    qDebug() << "<----- Set Receiver State: " << state;
}

void ReceiverController::setOutputDevice(AudioDirectionValue dir, QString devID)
{
    qDebug() << "<----- Set Audio OutputDevice: " << devID;
}

void ReceiverController::setSyncMode(SyncMode syncMode) const
{
    qDebug() << "<----- Set Receiver Sync Mode: " << syncMode;
}
