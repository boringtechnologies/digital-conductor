//*****************************************************************************
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

    void updatePeerList();

    void onGrpcMessage(const std::string &from, const std::string &to, const std::string &msg);

    void onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr);
    void onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr);
    void onGatheringDone(juice_agent_t *agent, void *user_ptr);
    void onVideoRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);
    void onAudioRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);

    dc::JuiceClientPtr getP2PClient(const std::string &id);
    dc::ReceiverPtr getVideoReceiver(const std::string &id);
    dc::ReceiverPtr getAudioReceiver(const std::string &id);

  protected:
    AppController *m_o;

    AppController::State m_state;

    std::string m_target;
    std::string m_user;
    std::string m_group;
    std::string m_type;

    dc::GRpcClientPtr m_rpcClient;

    // TODO mutex?
    QTimer m_updatePeersTimer;
    dc::PeerType m_peerType;
    std::vector<dc::Peer> m_rpcPeers;

    std::map<std::string, dc::JuiceClientPtr> m_p2pConnections;

    // pair class - device description
    std::map<std::string, dc::media::DeviceDescr> m_currentDevices;

    // sender
    media::ClockInfo m_mediaclock = {"66.228.38.186", 10000};
    dc::SenderPtr m_videosender;
    dc::SenderPtr m_audiosender;

    // receiver
    bool m_syncMode                  = true;
    media::ClockInfo m_receiverclock = {"66.228.38.186", 10000};
    std::chrono::duration<int, std::milli> m_delay{500};
    std::map<std::string, dc::ReceiverPtr> m_videoreceivers;
    std::map<std::string, dc::ReceiverPtr> m_audioreceivers;
};

//******************************************************************************
//******************************************************************************
AppController::impl::impl(AppController *owner) : m_o(owner), m_state(AppController::StateWelcome)
{
    connect(&m_updatePeersTimer, &QTimer::timeout, this, &impl::updatePeerList);
    m_updatePeersTimer.start(5000);
}

//*****************************************************************************
//*****************************************************************************
void AppController::impl::updatePeerList()
{
    if (m_rpcClient)
    {
        m_rpcPeers = m_rpcClient->getPeerList();

        // TODO check connections and connect to new peers if streaming
    }
}

//*****************************************************************************
//*****************************************************************************
void AppController::impl::onGrpcMessage(const std::string &from, const std::string &to, const std::string &msg)
{
    std::cout << "[appctrl] onGrpcMessage"
              << "Message from " << from << " --> " << msg << std::endl;

    // TODO move juice messages to another instance
    if (msg.rfind("JP2PI", 0) == 0)
    {
        // juice init connection, sdp
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (ptr)
        {
            std::cout << "[appctrl] onGrpcMessage juice already initialized " << from << std::endl;
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
        ptr->init(msg.substr(6));

        if (!m_rpcClient->p2pAccept(from, ptr->sdp()))
        {
            std::cout << "[appctrl] onGrpcMessage "
                      << "p2p not accepted " << from << std::endl;
            return;
        }

        m_p2pConnections[from] = ptr;
    }
    else if (msg.rfind("JP2PA", 0) == 0)
    {
        // juice connection accepted, sdp
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (!ptr)
        {
            std::cout << "[appctrl] onGrpcMessage "
                      << "juice not initialized " << from << std::endl;
            return;
        }

        ptr->connect(msg.substr(6));
    }
    else if (msg.rfind("JP2PC", 0) == 0)
    {
        // juice candidate
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (!ptr)
        {
            std::cout << "[appctrl] onGrpcMessage "
                      << "juice not initialized " << from << std::endl;
            return;
        }
        ptr->addRemote(msg.substr(6));
    }
    else if (msg.rfind("JP2PD", 0) == 0)
    {
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
        std::cout << "[appctrl] onGrpcMessage "
                  << "juice disconnect from " << from << " reason: " << msg.substr(6) << std::endl;

        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (ptr)
        {
            ptr->disconnect();
            ptr.reset();
        }

        m_p2pConnections.erase(from);
    }
    else
    {
        // TODO simple message
        std::cout << "[appctrl] onGrpcMessage "
                  << "msg from <" << from << "> " << msg << std::endl;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    std::string id          = caller->id();

    std::cout << "[appctrl] onStateChanged "
              << "P2P connection <" << state << "> " << juice_state_to_string(state) << std::endl;

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
            std::cout << "    no receiver named " << id << std::endl;
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
    std::cout << "[appctrl] onStateChanged handled" << std::endl;
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr)
{
    // std::cout << "onCandidate " << sdp << std::endl;

    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    if (m_rpcClient)
    {
        m_rpcClient->sendMessage(caller->id(), std::string("JP2PC ") + sdp);
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
            std::cout << "p2p disconnect error" << std::endl;
            continue;
        }

        it.second->disconnect();
    }
}

//******************************************************************************
//******************************************************************************
void AppController::init(const QString &userName, const QString &group, const QString &target)
{
    m_p->m_user   = userName.toStdString();
    m_p->m_group  = group.toStdString();
    m_p->m_target = target.toStdString();

    m_p->m_rpcClient.reset(new dc::GRpcClient(m_p->m_user, m_p->m_group,
                                              grpc::CreateChannel(m_p->m_target, grpc::InsecureChannelCredentials())));

    dc::PeerType type = dc::PeerType::UNKNOWN;
    if (!m_p->m_type.empty())
    {
        type = m_p->m_type == "r"    ? dc::PeerType::READER
               : m_p->m_type == "w"  ? dc::PeerType::WRITER
               : m_p->m_type == "rw" ? (dc::PeerType)(dc::PeerType::READER & dc::PeerType::WRITER)
                                     : dc::PeerType::UNKNOWN;
    }

    m_p->m_rpcClient->subscribe(type, std::bind(&impl::onGrpcMessage, m_p.get(), std::placeholders::_1,
                                                std::placeholders::_2, std::placeholders::_3));
}

//******************************************************************************
//******************************************************************************
std::vector<dc::Peer> AppController::getPeers() const
{
    return m_p->m_rpcPeers;
}

//******************************************************************************
//******************************************************************************
// std::shared_ptr<dc::GRpcClient> AppController::rpcClient()
//{
//    return m_p->m_rpcClient;
//}

//******************************************************************************
//******************************************************************************
AppController::State AppController::state() const
{
    return m_p->m_state;
}

//******************************************************************************
//******************************************************************************
QString AppController::group() const
{
    return QString::fromStdString(m_p->m_group);
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
    std::cout << "[appctrl] "
              << "sync mode " << (sync ? "on" : "off") << std::endl;
    m_p->m_syncMode = sync;
}

//******************************************************************************
//******************************************************************************
void AppController::selectDevice(const dc::media::DeviceDescr &dev)
{
    std::cout << "[appctrl] "
              << "selected " << dev.toStdString() << std::endl;
    m_p->m_currentDevices[dev.deviceClass] = dev;
}

//******************************************************************************
//******************************************************************************
void AppController::startStreaming()
{
    if (!m_p->m_rpcClient)
    {
        std::cerr << "[appctrl] startStreaming "
                  << "not subscribed" << std::endl;
        // TODO emit error
        return;
    }
    if (m_p->m_videosender)
    {
        std::cerr << "[appctrl] startStreaming "
                  << "already started" << std::endl;
        // TODO emit error
        return;
    }
    if (!m_p->m_videoreceivers.empty())
    {
        std::cerr << "[appctrl] startStreaming "
                  << "already receiving" << std::endl;
        // TODO emit error
        return;
    }

    std::string videoDeviceClass = "Video/Source";
    if (m_p->m_currentDevices.count(videoDeviceClass) == 0)
    {
        std::cerr << "[appctrl] startStreaming "
                  << "no video capture device selected" << std::endl;
        // TODO emit error
        return;
    }

    std::string audioDeviceClass = "Audio/Source";
    if (m_p->m_currentDevices.count(audioDeviceClass) == 0)
    {
        std::cerr << "[appctrl] startStreaming "
                  << "no audio capture device selected" << std::endl;
        // TODO emit error
        return;
    }

    const dc::media::DeviceDescr &videoDevDescr = m_p->m_currentDevices[videoDeviceClass];
    media::CaptureDevice videoDevice{videoDevDescr.internalName, videoDevDescr.internalParams};

    const dc::media::DeviceDescr &audioDevDescr = m_p->m_currentDevices[audioDeviceClass];
    media::CaptureDevice audioDevice{audioDevDescr.internalName, audioDevDescr.internalParams};

    // start senders
    m_p->m_videosender.reset(new media::Sender(m_p->m_user, media::MediaType::Video, videoDevice, m_p->m_mediaclock));
    m_p->m_videosender->start();

    m_p->m_audiosender.reset(new media::Sender(m_p->m_user, media::MediaType::Audio, audioDevice, m_p->m_mediaclock));
    m_p->m_audiosender->start();

    m_p->m_peerType = (dc::PeerType)(m_p->m_peerType | dc::PeerType::WRITER);
    m_p->m_rpcClient->updatePeer(m_p->m_peerType);

    // connect peers
    //    for (const dc::Peer & p : m_p->m_rpcPeers)
    //    {
    //        dc::JuiceClientPtr ptr = m_p->getP2PClient(p.id());
    //        if (ptr)
    //        {
    //            // already connected, start stream
    //            m_p->m_mediasender->addReceiver(p.id(), [ptr](const uint8_t *data, size_t len)
    //            {
    //                ptr->sendMessage(data, len);
    //                // std::cout << len << " bytes to " << id << std::endl;
    //            });

    //            continue;
    //        }

    //        // establish connection
    //        ptr.reset(new dc::JuiceClient(p.id(),
    //                                      std::bind(&impl::onStateChanged,  m_p.get(), std::placeholders::_1,
    //                                      std::placeholders::_2, std::placeholders::_3), std::bind(&impl::onCandidate,
    //                                      m_p.get(), std::placeholders::_1, std::placeholders::_2,
    //                                      std::placeholders::_3), std::bind(&impl::onGatheringDone, m_p.get(),
    //                                      std::placeholders::_1, std::placeholders::_2), std::bind(&impl::onRecv,
    //                                      m_p.get(), std::placeholders::_1, std::placeholders::_2,
    //                                      std::placeholders::_3, std::placeholders::_4)));
    //        ptr->init();

    //        if (!m_p->m_rpcClient->p2pInit(p.id(), ptr->sdp()))
    //        {
    //            std::cout << "p2p init error" << std::endl;
    //            continue;
    //        }

    //        m_p->m_p2pConnections[p.id()] = ptr;
    //    }
}

//******************************************************************************
//******************************************************************************
void AppController::receiveStream()
{
    if (m_p->m_videosender)
    {
        std::cerr << "[appctrl] receiveStream "
                  << "i'm a sender, abort receiving" << std::endl;
        // TODO emit error
        return;
    }
    if (!m_p->m_videoreceivers.empty())
    {
        std::cerr << "[appctrl] receiveStream "
                  << "already receiving, abort receiving" << std::endl;
        // TODO emit error
        return;
    }

    m_p->m_peerType = (dc::PeerType)(m_p->m_peerType | dc::PeerType::READER);
    m_p->m_rpcClient->updatePeer(m_p->m_peerType);

    // TODO one writer temporary
    for (const dc::Peer &peer : m_p->m_rpcPeers)
    {
        if (!(peer.type() & dc::PeerType::WRITER))
        {
            // not a writer
            continue;
        }

        // dc::JuiceClientPtr ptr = m_p->getP2PClient(id);
        dc::JuiceClientPtr ptr = m_p->getP2PClient(peer.id());
        if (ptr)
        {
            // disconnect
            if (!m_p->m_rpcClient->p2pDisconnect(peer.id(), "get stream"))
            {
                std::cout << "[appctrl] receiveStream "
                          << "p2p disconnect error" << std::endl;
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
        m_p->m_videoreceivers[peer.id()].reset(new media::Receiver(peer.id(), media::MediaType::Video, videodevice,
                                                                   m_p->m_receiverclock, m_p->m_delay, m_p->m_syncMode,
                                                                   [ptr](const uint8_t *data, size_t len) {
                                                                       ptr->sendVideo(data, len);
                                                                       // std::cout << len << " bytes to " << id <<
                                                                       // std::endl;
                                                                   }));

        media::PlaybackDevice audiodevice;
        m_p->m_audioreceivers[peer.id()].reset(new media::Receiver(peer.id(), media::MediaType::Audio, audiodevice,
                                                                   m_p->m_receiverclock, m_p->m_delay, m_p->m_syncMode,
                                                                   [ptr](const uint8_t *data, size_t len) {
                                                                       ptr->sendAudio(data, len);
                                                                       // std::cout << len << " bytes to " << id <<
                                                                       // std::endl;
                                                                   }));

        // start connection
        if (!m_p->m_rpcClient->p2pInit(peer.id(), ptr->sdp()))
        {
            std::cout << "[appctrl] receiveStream "
                      << "p2p init error with " << peer.id() << std::endl;
            continue;
        }

        m_p->m_p2pConnections[peer.id()] = ptr;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::stopStream()
{
    m_p->m_peerType = (dc::PeerType)(m_p->m_peerType & ~dc::PeerType::WRITER);
    m_p->m_rpcClient->updatePeer(m_p->m_peerType);

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

            if (!m_p->m_rpcClient->p2pDisconnect(p.id(), "stopped"))
            {
                std::cout << "[appctrl] stopStream "
                          << "p2p disconnect error" << std::endl;
                continue;
            }

            std::async(std::launch::async, [&] {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                if (dc::JuiceClientPtr ptr = m_p->getP2PClient(p.id()))
                {
                    ptr->disconnect();
                    // m_p->m_p2pConnections.erase(p.id());
                }
            });
        }

        m_p->m_p2pConnections.clear();
    }

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
}
