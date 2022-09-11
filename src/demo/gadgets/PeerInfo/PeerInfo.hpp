#pragma once

#include <QObject>
#include <QString>

class PeerInfo
{
    Q_GADGET

    struct PeerInfoStruct
    {
        QString id {};

        bool isSender {};
        bool isReceiver {};

        PeerInfoStruct(QString idVal, bool isSenderVal, bool isReceiverVal)
            : id(idVal)
            , isSender(isSenderVal)
            , isReceiver(isReceiverVal)
        {
        }
        PeerInfoStruct() = default;
    };

    Q_PROPERTY(QString id READ getID WRITE setID USER true)
    Q_PROPERTY(bool isSender READ getIsSender WRITE setIsSender USER true)
    Q_PROPERTY(bool isReceiver READ getIsReceiver WRITE setIsReceiver USER true)

public:
    static PeerInfo toQt(PeerInfo::PeerInfoStruct);
    static PeerInfo::PeerInfoStruct fromQt(PeerInfo);

    PeerInfo(QString id, bool isSender, bool isReceiver);
    PeerInfo() = default;

    QString getID() const;
    void setID(QString);
    
    bool getIsSender() const;
    void setIsSender(bool);

    bool getIsReceiver() const;
    void setIsReceiver(bool);

private:
    PeerInfo::PeerInfoStruct info_;
};

Q_DECLARE_METATYPE(PeerInfo)
