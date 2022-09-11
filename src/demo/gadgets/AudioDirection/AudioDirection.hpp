#pragma once

#include <QObject>

class AudioDirection
{
    Q_GADGET

public:
    enum AudioDirectionValue
    {
        Output,
        Input,
    };
    Q_ENUM(AudioDirectionValue)

};

using AudioDirectionValue = AudioDirection::AudioDirectionValue;

Q_DECLARE_METATYPE(AudioDirection)
