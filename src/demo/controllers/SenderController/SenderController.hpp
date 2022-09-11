#pragma once

#include "SenderController/SenderController.hpp"
#include "ControllerBase/ControllerBase.hpp"
#include "AudioDirection/AudioDirection.hpp"

#include <QList>

class SenderController : public QObject,
                         public ControllerBase<SenderController>
{
    Q_OBJECT

public:
    enum class State
    {
        Running,
        Stopped,
    };
    Q_ENUM(State)

    Q_INVOKABLE void setState(State state);

    Q_INVOKABLE void setInputDevice(AudioDirectionValue dir, QString devID);

private:
    friend class ControllerBase;

    SenderController();
    ~SenderController() override;
};
