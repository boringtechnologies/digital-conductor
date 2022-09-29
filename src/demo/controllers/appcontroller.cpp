//*****************************************************************************
//*****************************************************************************

#include "appcontroller.h"

#include "licensing/LicenseManager.hpp"

#include "client/grpcclient.h"
#include "client/juiceclient.h"

#undef signals // for gstreamer
#include "sync.hpp"

#include <QGuiApplication>
#include <QClipboard>
#include <QDebug>
#include <QTimer>

#include <memory>

//******************************************************************************
//******************************************************************************
namespace dc
{
} // namespace dc

//******************************************************************************
//******************************************************************************
class AppController::impl : public QObject
{
    friend class AppController;

protected:
    impl(AppController * owner);

    void updatePeerList();

    void onGrpcMessage(const std::string & from, const std::string & to, const std::string & msg);

    void onStateChanged(juice_agent_t * agent, juice_state_t state, void * user_ptr);
    void onCandidate(juice_agent_t * agent, const char * sdp, void * user_ptr);
    void onGatheringDone(juice_agent_t * agent, void * user_ptr);
    void onRecv(juice_agent_t * agent, const char * data, size_t size, void * user_ptr);

    dc::JuiceClientPtr getP2PClient(const std::string & id);

protected:
    AppController       * m_o;

    AppController::State  m_state;

    std::string           m_target;
    std::string           m_user;
    std::string           m_group;
    std::string           m_type;

    dc::GRpcClientPtr     m_rpcClient;

    // TODO mutex?
    QTimer                m_updatePeersTimer;
    std::vector<dc::Peer> m_rpcPeers;

    std::map<std::string, dc::JuiceClientPtr> m_p2pConnections;

    // sender
    media::ClockInfo                 m_mediaclock = {"66.228.38.186", 10000};
    media::MediaType                 m_mediatype = media::MediaType::Video;
    media::CaptureDevice             m_captureDevice;
    std::shared_ptr<media::Sender>   m_mediasender;

    // receiver
    media::PlaybackDevice            m_playbackDevice;
    media::ClockInfo                 m_receiverclock = {"66.228.38.186", 10000};
    std::chrono::duration<int>       m_delay;

    std::string                      m_receivedFromId;
    std::shared_ptr<media::Receiver> m_mediareceiver;
};

//******************************************************************************
//******************************************************************************
AppController::impl::impl(AppController * owner)
    : m_o(owner)
    , m_state(AppController::StateWelcome)
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
void AppController::impl::onGrpcMessage(const std::string & from, const std::string & to, const std::string & msg)
{
    // std::cout << "Message from " << from << " --> " << msg << std::endl;

    // TODO move juice messages to another instance
    if (msg.rfind("JP2PI", 0) == 0)
    {
        // juice init connection, sdp
        dc::JuiceClientPtr ptr = getP2PClient(from);
        if (ptr)
        {
            std::cout << "juice already initialized " << from << std::endl;
            return;
        }

        ptr.reset(new dc::JuiceClient(from,
                                      std::bind(&impl::onStateChanged,  this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onCandidate,     this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onGatheringDone, this, std::placeholders::_1, std::placeholders::_2),
                                      std::bind(&impl::onRecv,          this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
        ptr->init(msg.substr(6));

        if (!m_rpcClient->p2pAccept(from, ptr->sdp()))
        {
            std::cout << "p2p not accepted " << from << std::endl;
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
            std::cout << "juice not initialized " << from << std::endl;
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
            std::cout << "juice not initialized " << from << std::endl;
            return;
        }
        ptr->addRemote(msg.substr(6));
    }
    else if (msg.rfind("JP2PD", 0) == 0)
    {
        // juice disconnect
        if (m_mediasender)
        {
            m_mediasender->removeReceiver(from);
        }
        if (m_mediareceiver)
        {
            m_mediareceiver->stop();
            m_mediareceiver.reset();
            m_receivedFromId.clear();
        }
        std::cout << "juice disconnect from " << from << " reason: " << msg.substr(6) << std::endl;

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
        std::cout << "msg from <" << from << "> " << msg << std::endl;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onStateChanged(juice_agent_t * agent, juice_state_t state, void * user_ptr)
{
    dc::JuiceClient * caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    std::string id = caller->id();

    // std::cout << "onStateChanged " << state << " " << juice_state_to_string(state) << std::endl;
    std::cout << "P2P connection <" << state << "> " << juice_state_to_string(state) << std::endl;

    if (state == JUICE_STATE_COMPLETED)
    {
        std::cout << "P2P connection, " << id << " connected" << std::endl;

        if (m_mediasender)
        {
            dc::JuiceClientPtr ptr = getP2PClient(id);
            if (!ptr)
            {
                std::cout << "    no receiver named " << id << std::endl;
            }
            else
            {
                m_mediasender->addReceiver(id, [ptr](const uint8_t *data, size_t len)
                {
                    ptr->sendMessage(data, len);
                    // std::cout << len << " bytes to " << id << std::endl;
                });
            }
        }

        if (m_mediareceiver)
        {
            m_mediareceiver->start();
        }
    }
    else if (state == JUICE_STATE_DISCONNECTED)
    {
        if (m_mediasender)
        {
            m_mediasender->removeReceiver(id);
        }
        if (m_mediareceiver)
        {
            m_mediareceiver->stop();
            m_mediareceiver.reset();
            m_receivedFromId.clear();
        }

        dc::JuiceClientPtr ptr = getP2PClient(id);
        if (ptr)
        {
            ptr.reset();
        }
        m_p2pConnections.erase(id);

        std::cout << "P2P connection, " << id << " disconnected" << std::endl;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::impl::onCandidate(juice_agent_t * agent, const char * sdp, void * user_ptr)
{
    // std::cout << "onCandidate " << sdp << std::endl;

    dc::JuiceClient * caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    if (m_rpcClient)
    {
        m_rpcClient->sendMessage(caller->id(), std::string("JP2PC ") + sdp);
    }

}

//******************************************************************************
//******************************************************************************
void AppController::impl::onGatheringDone(juice_agent_t * agent, void * user_ptr)
{

}

//******************************************************************************
//******************************************************************************
void AppController::impl::onRecv(juice_agent_t * agent, const char * data, size_t size, void * user_ptr)
{
    dc::JuiceClient * caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    // std::cout << size << " bytes from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    if (m_mediareceiver)
    {
        m_mediareceiver->push(reinterpret_cast<const uint8_t *>(data), size);
    }
    else if (m_mediasender)
    {
        for (const auto &jc : m_p2pConnections)
        {
            if (jc.second.get() == caller)
            {
                m_mediasender->push(jc.first, reinterpret_cast<const uint8_t *>(data), size);
            }
        }
    }

}

//******************************************************************************
//******************************************************************************
dc::JuiceClientPtr AppController::impl::getP2PClient(const std::string & id)
{
    if (m_p2pConnections.count(id) == 0)
    {
        // std::cout << "message to unknown " << to << std::endl;
        return dc::JuiceClientPtr();
    }

    return m_p2pConnections[id];
}

//*****************************************************************************
//*****************************************************************************
AppController::AppController()
    : m_p(new impl(this))
{

}

//******************************************************************************
//******************************************************************************
// static
AppController * AppController::instance(QQmlEngine *, QJSEngine *)
{
    static AppController * c = new AppController;
    return c;
}

//*****************************************************************************
//*****************************************************************************
AppController::~AppController()
{
    if (m_p->m_mediareceiver)
    {
        m_p->m_mediareceiver->stop();
        m_p->m_mediareceiver.reset();
        m_p->m_receivedFromId.clear();
    }

    if (m_p->m_mediasender)
    {
        m_p->m_mediasender->stop();
        m_p->m_mediasender.reset();
    }

    for (const std::pair<std::string, dc::JuiceClientPtr> & it : m_p->m_p2pConnections)
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
void AppController::init(const QString & userName, const QString & group, const QString & target)
{
    m_p->m_user   = userName.toStdString();
    m_p->m_group  = group.toStdString();
    m_p->m_target = target.toStdString();

    m_p->m_rpcClient.reset(new dc::GRpcClient(m_p->m_user, m_p->m_group, grpc::CreateChannel(m_p->m_target, grpc::InsecureChannelCredentials())));

    uint32_t type = 3;
    if (!m_p->m_type.empty())
    {
        type = m_p->m_type == "r"  ? 1 :
               m_p->m_type == "w"  ? 2 :
               m_p->m_type == "rw" ? 3 : 0;
    }

    m_p->m_rpcClient->subscribe(type, std::bind(&impl::onGrpcMessage, m_p.get(),
                                                std::placeholders::_1,
                                                std::placeholders::_2,
                                                std::placeholders::_3));
}

//******************************************************************************
//******************************************************************************
std::vector<dc::Peer> AppController::getPeers() const
{
    return m_p->m_rpcPeers;
}

//******************************************************************************
//******************************************************************************
//std::shared_ptr<dc::GRpcClient> AppController::rpcClient()
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
    m_p->m_state = licenseManager.LicenseCheck() ? StateCompleted
                                                 : StateRegisterLicense;

    emit stateChanged();
}

//******************************************************************************
//******************************************************************************
void AppController::copyToClipboard(QString data)
{
    QClipboard * clipboard = QGuiApplication::clipboard();
    clipboard->setText(data);
}

//******************************************************************************
//******************************************************************************
void AppController::startStreaming()
{
    if (!m_p->m_rpcClient)
    {
        std::cout << "not subscribed" << std::endl;
        // TODO emit error
        return;
    }
    if (m_p->m_mediasender)
    {
        std::cout << "already started" << std::endl;
        // TODO emit error
        return;
    }
    if (m_p->m_mediareceiver)
    {
        std::cout << "already receiving" << std::endl;
        // TODO emit error
        return;
    }

    // start sender
    m_p->m_captureDevice.name = "/dev/video0";
    m_p->m_mediasender.reset(new media::Sender(m_p->m_user, m_p->m_mediatype, m_p->m_captureDevice, m_p->m_mediaclock));
    m_p->m_mediasender->start();

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
//                                      std::bind(&impl::onStateChanged,  m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
//                                      std::bind(&impl::onCandidate,     m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
//                                      std::bind(&impl::onGatheringDone, m_p.get(), std::placeholders::_1, std::placeholders::_2),
//                                      std::bind(&impl::onRecv,          m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
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
    if (m_p->m_mediasender)
    {
        std::cout << "i'm a sender" << std::endl;
        // TODO emit error
        return;
    }
    if (m_p->m_mediareceiver)
    {
        std::cout << "already receiving" << std::endl;
        // TODO emit error
        return;
    }

    // TODO one writer temporary
    for (const dc::Peer & peer : m_p->m_rpcPeers)
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
                std::cout << "p2p disconnect error" << std::endl;
                continue;
            }

            m_p->m_p2pConnections.erase(peer.id());

            ptr->disconnect();
            ptr.reset();
        }

        // connection to sender
        ptr.reset(new dc::JuiceClient(peer.id(),
                                      std::bind(&impl::onStateChanged,  m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onCandidate,     m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onGatheringDone, m_p.get(), std::placeholders::_1, std::placeholders::_2),
                                      std::bind(&impl::onRecv,          m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
        ptr->init();

        bool synchronize = true;
        m_p->m_receivedFromId = peer.id();
        m_p->m_mediareceiver.reset(new media::Receiver(peer.id(), m_p->m_mediatype, m_p->m_playbackDevice, m_p->m_receiverclock, m_p->m_delay, synchronize,
                    [ptr](const uint8_t *data, size_t len)
                    {
                        ptr->sendMessage(data, len);
                        // std::cout << len << " bytes to " << id << std::endl;
                    }));

        // start connection
        if (!m_p->m_rpcClient->p2pInit(peer.id(), ptr->sdp()))
        {
            std::cout << "p2p init error with " << peer.id() << std::endl;
            continue;
        }

        m_p->m_p2pConnections[peer.id()] = ptr;

        // TODO temporary one sender
        break;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::stopStream()
{
    // TODO stop media for all connections
    if (m_p->m_mediasender)
    {
        m_p->m_mediasender->stop();
        m_p->m_mediasender.reset();

        for (const dc::Peer & p : m_p->m_rpcPeers)
        {

            if (!m_p->m_rpcClient->p2pDisconnect(p.id(), "stopped"))
            {
                std::cout << "p2p disconnect error" << std::endl;
                continue;
            }

            dc::JuiceClientPtr ptr = m_p->getP2PClient(p.id());
            if (ptr)
            {
                ptr->disconnect();
                // m_p->m_p2pConnections.erase(p.id());
            }
        }

        m_p->m_p2pConnections.clear();
    }
    if (m_p->m_mediareceiver)
    {
        m_p->m_mediareceiver->stop();
        m_p->m_mediareceiver.reset();

        dc::JuiceClientPtr ptr = m_p->getP2PClient(m_p->m_receivedFromId);
        if (ptr)
        {
            m_p->m_rpcClient->p2pDisconnect(m_p->m_receivedFromId, "closed");

            ptr->disconnect();
            m_p->m_p2pConnections.erase(m_p->m_receivedFromId);
        }
        m_p->m_receivedFromId.clear();
    }
}
