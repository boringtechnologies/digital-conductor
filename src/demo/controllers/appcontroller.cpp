//*****************************************************************************
//*****************************************************************************

#include "appcontroller.h"

#include "licensing/LicenseManager.hpp"

#include "client/grpcclient.h"
#include "client/juiceclient.h"

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
//        if (mediasender)
//        {
//            mediasender->removeReceiver(from);
//        }
//        std::cout << "juice disconnect from " << from << " reason: " << msg.substr(6) << std::endl;
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

//        if (!mediasender)
//        {
//            std::cout << "    no media" << std::endl;
//        }
//        else
//        {
//            if (jcclients.find(id) == jcclients.end() || !jcclients[id])
//            {
//                std::cout << "    no receiver named " << id << std::endl;
//            }
//            else
//            {
//                mediasender->addReceiver(id, [id](const uint8_t *data, size_t len)
//                {
//                    jcclients[id]->sendMessage(data, len);
//                    // std::cout << len << " bytes to " << id << std::endl;
//                });
//            }
//        }
    }
    else if (state == JUICE_STATE_DISCONNECTED)
    {
//        if (mediasender)
//        {
//            mediasender->removeReceiver(id);
//        }
//        if (mediareceiver)
//        {
//            mediareceiver->stop();
//            mediareceiver.reset();
//        }
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
    for (const dc::Peer & p : m_p->m_rpcPeers)
    {
        dc::JuiceClientPtr ptr;
        ptr.reset(new dc::JuiceClient(p.id(),
                                      std::bind(&impl::onStateChanged,  m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onCandidate,     m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
                                      std::bind(&impl::onGatheringDone, m_p.get(), std::placeholders::_1, std::placeholders::_2),
                                      std::bind(&impl::onRecv,          m_p.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4)));
        ptr->init();

        if (!m_p->m_rpcClient->p2pInit(p.id(), ptr->sdp()))
        {
            std::cout << "p2p init error" << std::endl;
            continue;
        }

        m_p->m_p2pConnections[p.id()] = ptr;
    }
}

//******************************************************************************
//******************************************************************************
void AppController::receiveStream()
{
    qDebug() << Q_FUNC_INFO;
}

//******************************************************************************
//******************************************************************************
void AppController::stopStream()
{
    // TODO stop media for all connections

    for (const dc::Peer & p : m_p->m_rpcPeers)
    {

        if (!m_p->m_rpcClient->p2pDisconnect(p.id(), "user request"))
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
