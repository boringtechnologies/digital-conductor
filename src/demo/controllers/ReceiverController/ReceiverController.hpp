#pragma once

#include "ReceiverController/ReceiverController.hpp"
#include "ControllerBase/ControllerBase.hpp"
#include "AudioDirection/AudioDirection.hpp"

#include <QList>

class ReceiverController : public QObject,
                           public ControllerBase<ReceiverController>
{
    Q_OBJECT

public:
    enum class State
    {
        Running,
        Stopped,
    };
    Q_ENUM(State)

    enum class SyncMode
    {
        On,
        Off,
    };
    Q_ENUM(SyncMode)

    Q_INVOKABLE void setState(State state) const;

    Q_INVOKABLE void setOutputDevice(AudioDirectionValue dir, QString devID);

    Q_INVOKABLE void setSyncMode(SyncMode syncMode) const;

private:
    friend class ControllerBase;

    ReceiverController();
    ~ReceiverController() override;
};
