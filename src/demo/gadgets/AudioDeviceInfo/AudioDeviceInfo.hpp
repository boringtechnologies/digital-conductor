#pragma once

#include <QObject>
#include <QString>

class AudioDeviceInfo
{
    Q_GADGET

    struct DeviceInfoStruct
    {
        //! Device identifier.
        std::string id {};

        //! Human-readable one-line device label.
        std::string label;

        DeviceInfoStruct(QString idVal, QString labelVal)
            : id(idVal.toStdString())
            , label(labelVal.toStdString())
        {
        }
        DeviceInfoStruct() = default;
    };

    Q_PROPERTY(QString id READ getID WRITE setID USER true)
    Q_PROPERTY(QString label READ getLabel WRITE setLabel USER true)

public:
    static AudioDeviceInfo toQt(AudioDeviceInfo::DeviceInfoStruct);
    static AudioDeviceInfo::DeviceInfoStruct fromQt(AudioDeviceInfo);

    AudioDeviceInfo(QString id, QString label);
    AudioDeviceInfo() = default;

    QString getID() const;
    void setID(QString);

    QString getLabel() const;
    void setLabel(QString);

private:
    AudioDeviceInfo::DeviceInfoStruct info_;
};

Q_DECLARE_METATYPE(AudioDeviceInfo)
