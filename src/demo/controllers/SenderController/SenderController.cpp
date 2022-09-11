#include "SenderController/SenderController.hpp"

#include <QDebug>

SenderController::SenderController()
{
}

SenderController::~SenderController()
{
}

void SenderController::setState(State state)
{
    qDebug() << "<----- Set Sender State: " << state;
}

void SenderController::setInputDevice(AudioDirectionValue dir, QString devID)
{
    qDebug() << "<----- Set Audio OutputDevice: " << devID;
}
