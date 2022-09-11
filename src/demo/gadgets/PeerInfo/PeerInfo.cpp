#include "./PeerInfo.hpp"

PeerInfo::PeerInfo(QString id, bool isSender, bool isReceiver)
{
    setID(id);
    setIsSender(isSender);
    setIsReceiver(isReceiver);
}

PeerInfo PeerInfo::toQt(PeerInfo::PeerInfoStruct info)
{
    PeerInfo ret;
    ret.info_ = info;
    return ret;
}

PeerInfo::PeerInfoStruct PeerInfo::fromQt(PeerInfo info)
{
    return info.info_;
}

QString PeerInfo::getID() const
{
    return info_.id;
}

void PeerInfo::setID(QString id)
{
    info_.id = id;
}

bool PeerInfo::getIsSender() const
{
    return info_.isSender;
}

void PeerInfo::setIsSender(bool isSender)
{
    info_.isSender = isSender;
}

bool PeerInfo::getIsReceiver() const
{
    return info_.isReceiver;
}

void PeerInfo::setIsReceiver(bool isReceiver)
{
    info_.isReceiver = isReceiver;
}
