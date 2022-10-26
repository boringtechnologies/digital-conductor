#pragma once

#include <QObject>

class DeviceDirection
{
    Q_GADGET

  public:
    enum DeviceDirectionValue
    {
        Output,
        Input,
    };
    Q_ENUM(DeviceDirectionValue)
};

using DeviceDirectionValue = DeviceDirection::DeviceDirectionValue;

Q_DECLARE_METATYPE(DeviceDirection)
