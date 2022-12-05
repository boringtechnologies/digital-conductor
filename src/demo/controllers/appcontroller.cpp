src/demo/controllers/appcontroller.cpp//*****************************************************************************
//*****************************************************************************

#include "appcontroller.h"

#include "licensing/LicenseManager.hpp"

#include "core/grpcclient.h"
#include "core/juiceclient.h"
#include "core/permissionmgr.hpp"

#undef signals // for gstreamer
#include "core/media/sync.hpp"

#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QTimer>

#include <fmt/core.h>

#include <chrono>
#include <future>
#include <memory>
#include <thread>

//******************************************************************************
//******************************************************************************
namespace dc
{

typedef std::shared_ptr<::media::Sender> SenderPtr;
typedef std::shared_ptr<::media::Receiver> ReceiverPtr;

} // namespace dc

//******************************************************************************
//******************************************************************************
class AppController::impl : public QObject
{
    friend class AppController;

  protected:
    impl(AppController *owner);

    void onGrpcMessage(const dc::Message &message);

    void onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr);
    void onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr);
    void onGatheringDone(juice_agent_t *agent, void *user_ptr);
    void onVideoRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);
    void onAudioRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);

    dc::JuiceClientPtr getP2PClient(const std::string &id);
    dc::ReceiverPtr getVideoReceiver(const std::string &id);
    dc::ReceiverPtr getAudioReceiver(const std::string &id);

  protected:
    static constexpr const char *networkClockHost = "66.228.38.186";
    static constexpr size_t networkClockPort      = 10000;

    AppController *m_o;

    AppController::State m_state;

    dc::GRpcClientPtr m_rpcClient;
    std::vector<dc::Peer> m_rpcPeers;

    std::map<std::string, dc::JuiceClientPtr> m_p2pConnections;

    // pair class - device description
    std::map<std::string, dc::media::DeviceDescr> m_currentDevices;

    // sender
    media::ClockInfo m_mediaclock = {networkClockHost, networkClockPort};
    dc::SenderPtr m_videosender;
    dc::SenderPtr m_audiosender;

    // receiver
    bool m_recieving                 = false;
    bool m_syncMode                  = true;
    media::ClockInfo m_receiverclock = {networkClockHost, networkClockPort};
    std::chrono::duration<int, std::milli> m_delay{200};
    std::map<std::string, dc::ReceiverPtr> m_videoreceivers;
    std::map<std::string, dc::ReceiverPtr> m_audioreceivers;
};

//******************************************************************************
//******************************************************************************
AppController::impl::impl(AppController *owner) : m_o(owner), m_state(AppController::StateWelcome)
{
}

//*****************************************************************************
//*****************************************************************************
void AppController::impl::onGrpcMessage(const dc::Message &msg)
{
    const std::string &from                                = msg.from();
    const std::string &to                                  = msg.to();
    const dc::MessageType type                             = msg.type();
    const std::map<dc::MessageType, std::string> typeNames = {
        {dc::P2PGENERIC, "P2PGENERIC"}, {dc::P2PLIST, "P2PLIST"},           {dc::P2PINIT, "P2PINIT"},
        {dc::P2PACCEPT, "P2PACCEPT"},   {dc::P2PCANDIDATE, "P2PCANDIDATE"}, {dc::P2PDISCONNECT, "P2PDISCONNECT"}};

    fmt::print("[appctrl] {}[{}] from {} --> {}\n", __func__, typeNames.at(type), from, to);

    switch (type)
    {
    case dc::P2PINIT: {
        // juice init connection, sdp
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (ptr)
        {
            fmt::print(stderr, "[appctrl] {} juice already initialized for {}\n", __func__, from);
            return;
        }

        ptr.reset(new dc::JuiceClient(
            from,
            std::bind(&impl::onStateChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&impl::onCandidate, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&impl::onGatheringDone, this, std::placeholders::_1, std::placeholders::_2),
            std::bind(&impl::onVideoRecv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                      std::placeholders::_4),
            std::bind(&impl::onAudioRecv, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                      std::placeholders::_4)));
        ptr->init(msg.data());

        if (!m_rpcClient->p2pAccept(from, ptr->sdp()))
        {
            fmt::print(stderr, "[appctrl] {} p2p not accepted from {}\n", __func__, from);
            return;
        }

        m_p2pConnections[from] = ptr;
    }
    break;
    case dc::P2PACCEPT: {
        // juice connection accepted, sdp
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (!ptr)
        {
            fmt::print(stderr, "[appctrl] {} juice not initialized for {}\n", __func__, from);
            return;
        }

        ptr->connect(msg.data());
    }
    break;
    case dc::P2PCANDIDATE: {
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (!ptr)
        {
            fmt::print(stderr, "[appctrl] {} juice not initialized for {}\n", __func__, from);
            return;
        }

        ptr->addRemote(msg.data());
    }
    break;
    case dc::P2PDISCONNECT: {
        // juice disconnect
        if (m_videosender)
        {
            m_videosender->removeReceiver(from);
        }
        if (m_audiosender)
        {
            m_audiosender->removeReceiver(from);
        }
        if (dc::ReceiverPtr res = getVideoReceiver(from))
        {
            m_videoreceivers.erase(from);
            res->stop();
            res.reset();
        }
        if (dc::ReceiverPtr res = getAudioReceiver(from))
        {
            m_audioreceivers.erase(from);
            res->stop();
            res.reset();
        }
        fmt::print("[appctrl] {} juice disconnect from: {} reason: {}\n", __func__, from, msg.data());

        if (dc::JuiceClientPtr ptr = getP2PClient(from))
        {
            ptr->disconnect();
            ptr.reset();
        }

        m_p2pConnections.erase(from);
    }
    break;
    case dc::P2PGENERIC: {
        // simple message
        fmt::print("[appctrl] {} generic msg from: {} data: {}\n", __func__, from, msg.data());
    }
    break;

    case dc::P2PLIST: {
        const auto thisPeer = m_rpcClient->peer();
        const auto peerList = msg.peers().peers();
        std::vector<dc::Peer> peers;
        peers.reserve(peerList.size());
        bool startReceive = false;
        for (size_t i = 0; i < peerList.size(); i++)
        {
            const dc::Peer &peer = peerList.Get(i);

            // sanity check
            if (thisPeer.group() != peer.group())
            {
                fmt::print(stderr, "FIXME This filtration must be done on server\n");
                continue;
            }

            // skip self check
            if (thisPeer.id() == peer.id())
            {
                continue;
            }

            if (m_recieving && peer.type() & dc::PeerType::WRITER &&
                m_videoreceivers.find(peer.id()) == m_videoreceivers.end())
            {
                startReceive = true;
            }

            peers.push_back(peer);
        }
        m_rpcPeers = peers;

        if (startReceive)
        {
            m_o->receiveStream(false);
        }
    }
    break;

    default: {
        fmt::print(stderr, "[appctrl] onGrpcMessage unknown message with type: {}\n", msg.type());
    }
    }
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    std::string id          = caller->id();

    fmt::print("[appctrl] {} P2P connection <{}>\n", __func__, juice_state_to_string(state));

    if (state == JUICE_STATE_CONNECTED)
    {
        if (dc::JuiceClientPtr ptr = getP2PClient(id))
        {
            if (m_videosender)
            {
                m_videosender->addReceiver(id, [ptr](const uint8_t *data, size_t len) {
                    ptr->sendVideo(data, len);
                    // std::cout << len << " bytes to " << id << std::endl;
                });
            }
            if (m_audiosender)
            {
                m_audiosender->addReceiver(id, [ptr](const uint8_t *data, size_t len) {
                    ptr->sendAudio(data, len);
                    // std::cout << len << " bytes to " << id << std::endl;
                });
            }
        }
        else
        {
            fmt::print("[appctrl] no receiver named {}\n", id);
        }

        if (dc::ReceiverPtr res = getVideoReceiver(id))
        {
            res->start();
        }

        if (dc::ReceiverPtr res = getAudioReceiver(id))
        {
            res->start();
        }
    }
    else if (state == JUICE_STATE_COMPLETED)
    {
        // the best client found, probably we can adjust reviever to improve network perfomance
    }
    else if (state == JUICE_STATE_DISCONNECTED)
    {
        if (m_videosender)
        {
            m_videosender->removeReceiver(id);
        }
        if (m_audiosender)
        {
            m_audiosender->removeReceiver(id);
        }
        if (dc::ReceiverPtr res = getVideoReceiver(id))
        {
            m_videoreceivers.erase(id);
            res->stop();
            res.reset();
        }
        if (dc::ReceiverPtr res = getAudioReceiver(id))
        {
            m_audioreceivers.erase(id);
            res->stop();
            res.reset();
        }
        if (dc::JuiceClientPtr ptr = getP2PClient(id))
        {
            ptr.reset();
        }
        m_p2pConnections.erase(id);
    }
    fmt::print("[appctrl] {} finished\n", __func__);
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr)
{
    // std::cout << "onCandidate " << sdp << std::endl;

    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    if (m_rpcClient)
    {
        m_rpcClient->sendMessage(caller->id(), dc::P2PCANDIDATE, sdp);
    }
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onGatheringDone(juice_agent_t *agent, void *user_ptr)
{
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onVideoRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    // std::cout << size << " bytes of video from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    std::string id = caller->id();

    if (dc::ReceiverPtr ptr = getVideoReceiver(id))
    {
        ptr->push(reinterpret_cast<const uint8_t *>(data), size);
    }
    if (m_videosender)
    {
        m_videosender->push(id, reinterpret_cast<const uint8_t *>(data), size);
    }
}

void AppController::impl::onAudioRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    // std::cout << size << " bytes of audio from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    std::string id = caller->id();

    if (dc::ReceiverPtr ptr = getAudioReceiver(id))
    {
        ptr->push(reinterpret_cast<const uint8_t *>(data), size);
    }
    if (m_audiosender)
    {
        m_audiosender->push(id, reinterpret_cast<const uint8_t *>(data), size);
    }
}

//******************************************************************************
//******************************************************************************
dc::JuiceClientPtr AppController::impl::getP2PClient(const std::string &id)
{
    if (m_p2pConnections.count(id) == 0)
    {
        return dc::JuiceClientPtr();
    }

    return m_p2pConnections[id];
}

//*****************************************************************************
//*****************************************************************************
dc::ReceiverPtr AppController::impl::getVideoReceiver(const std::string &id)
{
    if (m_videoreceivers.count(id) == 0)
    {
        return dc::ReceiverPtr();
    }

    return m_videoreceivers[id];
}

//*****************************************************************************
//*****************************************************************************
dc::ReceiverPtr AppController::impl::getAudioReceiver(const std::string &id)
{
    if (m_audioreceivers.count(id) == 0)
    {
        return dc::ReceiverPtr();
    }

    return m_audioreceivers[id];
}

//*****************************************************************************
//*****************************************************************************
AppController::AppController() : m_p(new impl(this))
{
}

//******************************************************************************
//******************************************************************************
// static
AppController *AppController::instance(QQmlEngine *, QJSEngine *)
{
    static AppController *c = new AppController;
    return c;
}

//*****************************************************************************
//*****************************************************************************
AppController::~AppController()
{
    for (const std::pair<std::string, dc::ReceiverPtr> &r : m_p->m_videoreceivers)
    {
        r.second->stop();
        // r.second.reset();
    }
    m_p->m_videoreceivers.clear();
    for (const std::pair<std::string, dc::ReceiverPtr> &r : m_p->m_audioreceivers)
    {
        r.second->stop();
        // r.second.reset();
    }
    m_p->m_audioreceivers.clear();

    if (m_p->m_videosender)
    {
        m_p->m_videosender->stop();
        m_p->m_videosender.reset();
    }

    if (m_p->m_audiosender)
    {
        m_p->m_audiosender->stop();
        m_p->m_audiosender.reset();
    }

    for (const std::pair<std::string, dc::JuiceClientPtr> &it : m_p->m_p2pConnections)
    {
        if (!m_p->m_rpcClient->p2pDisconnect(it.first, "destroyed"))
        {
            continue;
        }

        it.second->disconnect();
    }
}

//******************************************************************************
//******************************************************************************
void AppController::init(const QString &userName, const QString &groupName, const QString &targetName,
                         const QString &rwMode)
{
    std::string user   = userName.toStdString();
    std::string group  = groupName.toStdString();
    std::string target = targetName.toStdString();
    std::string rw     = rwMode.toStdString();

    dc::PeerType peerType = dc::PeerType::UNKNOWN;
    if (!rw.empty())
    {
        peerType = rw == "r"    ? dc::PeerType::READER
                   : rw == "w"  ? dc::PeerType::WRITER
                   : rw == "rw" ? (dc::PeerType)(dc::PeerType::READER | dc::PeerType::WRITER)
                                : dc::PeerType::UNKNOWN;
    }

    m_p->m_rpcClient.reset(
        new dc::GRpcClient(user, group, peerType, grpc::CreateChannel(target, grpc::InsecureChannelCredentials())));

    m_p->m_rpcClient->subscribe(peerType, std::bind(&impl::onGrpcMessage, m_p.get(), std::placeholders::_1));
}

//******************************************************************************
//******************************************************************************
std::vector<dc::Peer> AppController::getPeers() const
{
    return m_p->m_rpcPeers;
}

//******************************************************************************
//******************************************************************************
QString AppController::group() const
{
    return QString::fromStdString(m_p->m_rpcClient->peer().group());
}

//******************************************************************************
//******************************************************************************
AppController::State AppController::state() const
{
    return m_p->m_state;
}

//******************************************************************************
//******************************************************************************
QString AppController::generateLicenseToken() const
{
    dc::LicenseManager licenseManager;
    return QString::fromStdString(licenseManager.GenerateDeviceToken());
}

//******************************************************************************
//******************************************************************************
bool AppController::syncModeEnabled() const
{
    return m_p->m_syncMode;
}

//******************************************************************************
//******************************************************************************
void AppController::registerLicense(const QString license)
{
    dc::LicenseManager licenseManager;
    if (licenseManager.RegisterLicense(license.toStdString()))
    {
        m_p->m_state = StateCompleted;
        emit stateChanged();
    }

    // TODO emit error message or show messagebox
}

//******************************************************************************
//******************************************************************************
void AppController::checkLicense()
{
    dc::LicenseManager licenseManager;
    m_p->m_state = licenseManager.LicenseCheck() ? StateCompleted : StateRegisterLicense;

    emit stateChanged();
}

//******************************************************************************
//******************************************************************************
void AppController::copyToClipboard(QString data)
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}

//******************************************************************************
//******************************************************************************
void AppController::onSenderViewReady()
{
    dc::PermissionManager mgr;
    mgr.requestMissingPermissions();
}

//******************************************************************************
//******************************************************************************
void AppController::setSyncMode(const bool sync)
{
    fmt::print("[appctrl] sync mode {}\n", sync ? "on" : "off");
    m_p->m_syncMode = sync;

    for (const dc::Peer &peer : m_p->m_rpcPeers)
    {
        if (!(peer.type() & dc::PeerType::WRITER) ||
            m_p->m_videoreceivers.find(peer.id()) == m_p->m_videoreceivers.end())
            continue;
        m_p->m_videoreceivers[peer.id()]->setSync(m_p->m_syncMode, m_p->m_delay, m_p->m_receiverclock);
        m_p->m_audioreceivers[peer.id()]->setSync(m_p->m_syncMode, m_p->m_delay, m_p->m_receiverclock);
    }
}

//******************************************************************************
//******************************************************************************
void AppController::selectDevice(const dc::media::DeviceDescr &dev)
{
    fmt::print("[appctrl] selected device: ", dev.toStdString());
    m_p->m_currentDevices[dev.deviceClass] = dev;
}

//******************************************************************************
//******************************************************************************
void AppController::startStreaming()
{
    if (!m_p->m_rpcClient)
    {
        fmt::print(stderr, "[appctrl] startStreaming not subscribed\n");
        // TODO emit error
        return;
    }
    if (m_p->m_videosender)
    {
        fmt::print(stderr, "[appctrl] startStreaming already started\n");
        // TODO emit error
        return;
    }
    if (!m_p->m_videoreceivers.empty())
    {
        fmt::print(stderr, "[appctrl] startStreaming already receiving\n");
        // TODO emit error
        return;
    }

    std::string videoDeviceClass = "Video/Source";
    if (m_p->m_currentDevices.count(videoDeviceClass) == 0)
    {
        fmt::print(stderr, "[appctrl] startStreaming no video capture device selected\n");
        // TODO emit error
        return;
    }

    std::string audioDeviceClass = "Audio/Source";
    if (m_p->m_currentDevices.count(audioDeviceClass) == 0)
    {
        fmt::print(stderr, "[appctrl] startStreaming no audio capture device selected\n");
        // TODO emit error
        return;
    }

    emit setSenderBusy(true);

    const dc::media::DeviceDescr &videoDevDescr = m_p->m_currentDevices[videoDeviceClass];
    media::CaptureDevice videoDevice{videoDevDescr.internalName, videoDevDescr.internalParams};

    const dc::media::DeviceDescr &audioDevDescr = m_p->m_currentDevices[audioDeviceClass];
    media::CaptureDevice audioDevice{audioDevDescr.internalName, audioDevDescr.internalParams};

    auto &rpcClient  = m_p->m_rpcClient;
    const auto &user = rpcClient->peer().id();

    // start senders
    m_p->m_videosender.reset(
        new media::Sender(user, media::MediaType::Video, videoDevice, m_p->m_mediaclock, [this, rpcClient]() {
            dc::PeerType peerType = (dc::PeerType)(rpcClient->peer().type() | dc::PeerType::WRITER);
            rpcClient->updatePeer(peerType);
            emit setSenderBusy(false);
        }));
    m_p->m_videosender->start();

    m_p->m_audiosender.reset(new media::Sender(user, media::MediaType::Audio, audioDevice, m_p->m_mediaclock));
    m_p->m_audiosender->start();
}

//******************************************************************************
//******************************************************************************
void AppController::receiveStream(bool recreate)
{
    fmt::print("[appctrl] receiveStream({})\n", recreate);

    if (m_p->m_videosender)
    {
        fmt::print(stderr, "[appctrl] receiveStream i'm a sender, abort receiving\n");
        // TODO emit error
        return;
    }
    if (!m_p->m_videoreceivers.empty())
    {
        fmt::print(stderr, "[appctrl] receiveStream already receiving abort receiving\n");
        // TODO emit error
        return;
    }

    emit setReceiverBusy(true);

    m_p->m_recieving = true;

    auto rpcClient        = m_p->m_rpcClient;
    dc::PeerType peerType = (dc::PeerType)(rpcClient->peer().type() | dc::PeerType::READER);
    rpcClient->updatePeer(peerType);

    for (const dc::Peer &peer : m_p->m_rpcPeers)
    {
        if (!(peer.type() & dc::PeerType::WRITER))
        {
            // not a writer
            continue;
        }

        if (!recreate && m_p->m_videoreceivers.find(peer.id()) != m_p->m_videoreceivers.end())
        {
            // already created
            continue;
        }

        dc::JuiceClientPtr ptr = m_p->getP2PClient(peer.id());
        if (ptr)
        {
            // disconnect
            if (!rpcClient->p2pDisconnect(peer.id(), "get stream"))
            {
                continue;
            }

            m_p->m_p2pConnections.erase(peer.id());

            ptr->disconnect();
            ptr.reset();
        }

        // connection to sender
        ptr.reset(new dc::JuiceClient(
            peer.id(),
            std::bind(&impl::onStateChanged, m_p.get(), std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3),
            std::bind(&impl::onCandidate, m_p.get(), std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3),
            std::bind(&impl::onGatheringDone, m_p.get(), std::placeholders::_1, std::placeholders::_2),
            std::bind(&impl::onVideoRecv, m_p.get(), std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3, std::placeholders::_4),
            std::bind(&impl::onAudioRecv, m_p.get(), std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3, std::placeholders::_4)));
        ptr->init();

        // no need to select anything for video
        media::PlaybackDevice videodevice;
        m_p->m_videoreceivers[peer.id()].reset(new media::Receiver(
            peer.id(), media::MediaType::Video, videodevice,
            [ptr](const uint8_t *data, size_t len) { ptr->sendVideo(data, len); },
            [this]() { emit setReceiverBusy(false); }));
        m_p->m_videoreceivers[peer.id()]->setSync(m_p->m_syncMode, m_p->m_delay, m_p->m_receiverclock);

        media::PlaybackDevice audiodevice;
        m_p->m_audioreceivers[peer.id()].reset(
            new media::Receiver(peer.id(), media::MediaType::Audio, audiodevice,
                                [ptr](const uint8_t *data, size_t len) { ptr->sendAudio(data, len); }));
        m_p->m_audioreceivers[peer.id()]->setSync(m_p->m_syncMode, m_p->m_delay, m_p->m_receiverclock);

        // start connection
        if (!rpcClient->p2pInit(peer.id(), ptr->sdp()))
        {
            fmt::print(stderr, "[appctrl] receiveStream p2pInit error for {}\n", peer.id());
            continue;
        }

        m_p->m_p2pConnections[peer.id()] = ptr;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::stopStream()
{
    fmt::print("[appctrl] stopStream()\n");

    auto rpcClient    = m_p->m_rpcClient;
    dc::PeerType type = (dc::PeerType)(rpcClient->peer().type() & ~dc::PeerType::WRITER);
    rpcClient->updatePeer(type);

    if (m_p->m_audiosender)
    {
        m_p->m_audiosender->stop();
        m_p->m_audiosender.reset();
    }
    if (m_p->m_videosender)
    {
        m_p->m_videosender->stop();
        m_p->m_videosender.reset();

        for (const dc::Peer &p : m_p->m_rpcPeers)
        {
            if (!rpcClient->p2pDisconnect(p.id(), "stopped"))
            {
                fmt::print(stderr, "[appctrl] stopStream p2p disconnect error\n");
                continue;
            }

            if (dc::JuiceClientPtr ptr = m_p->getP2PClient(p.id()))
            {
                ptr->disconnect();
            }
        }

        m_p->m_p2pConnections.clear();
    }
}

void AppController::stopReceive()
{
    emit setReceiverBusy(true);

    m_p->m_recieving = false;

    dc::PeerType type = (dc::PeerType)(m_p->m_rpcClient->peer().type() & ~dc::PeerType::READER);
    m_p->m_rpcClient->updatePeer(type);

    for (const std::pair<std::string, dc::ReceiverPtr> &r : m_p->m_videoreceivers)
    {
        if (r.second)
        {
            r.second->stop();
        }
        // r.second.reset();

        m_p->m_audioreceivers[r.first]->stop();

        dc::JuiceClientPtr ptr = m_p->getP2PClient(r.first);
        if (ptr)
        {
            m_p->m_rpcClient->p2pDisconnect(r.first, "closed");

            ptr->disconnect();
            m_p->m_p2pConnections.erase(r.first);
        }
    }
    m_p->m_audioreceivers.clear();
    m_p->m_videoreceivers.clear();

    emit setReceiverBusy(false);
}
