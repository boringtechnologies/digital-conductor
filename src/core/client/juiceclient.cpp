//******************************************************************************
//******************************************************************************

#include "juiceclient.h"

#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

#include <juice/juice.h>

//******************************************************************************
//******************************************************************************
namespace dc
{

//******************************************************************************
//******************************************************************************
class JuiceClient::impl
{
    friend class JuiceClient;

    impl(JuiceClient * owner, const std::string & id,
         on_state_changed_t onStateChangedHandler, on_candidate_t onCandidateHandler,
         on_gathering_done_t onGatheringDoneHandler, on_recv_t onReceiveHandler)
        : m_o(owner)
        , m_id(id)
        , m_onStateChangedHandler(onStateChangedHandler)
        , m_onCandidateHandler(onCandidateHandler)
        , m_onGatheringDoneHandler(onGatheringDoneHandler)
        , m_onReceiveHandler(onReceiveHandler)
    {
    }

    JuiceClient       * m_o     = nullptr;
    std::string         m_id;
    juice_agent_t     * m_agent = nullptr;
    juice_config_t      m_config;
    juice_turn_server_t m_turnServer;

    on_state_changed_t  m_onStateChangedHandler;
    on_candidate_t      m_onCandidateHandler;
    on_gathering_done_t m_onGatheringDoneHandler;
    on_recv_t           m_onReceiveHandler;

    std::atomic<bool>   m_destroymarker;
    std::mutex          m_sendLocker;
    std::thread         m_pinger;

    std::chrono::system_clock::time_point m_lastReceived;

    static void onStateChanged(juice_agent_t * agent, juice_state_t state, void * user_ptr);
    static void onRecv(juice_agent_t * agent, const char * data, size_t size, void * user_ptr);
};

//******************************************************************************
//******************************************************************************
JuiceClient::JuiceClient(const std::string & id,
                         on_state_changed_t onStateChangedHandler, on_candidate_t onCandidateHandler,
                         on_gathering_done_t onGatheringDoneHandler, on_recv_t onReceiveHandler)
    : m_p(new impl(this, id,
                   onStateChangedHandler, onCandidateHandler,
                   onGatheringDoneHandler, onReceiveHandler))
{
    m_p->m_destroymarker = false;
    m_p->m_pinger = std::thread([this]()
    {
        const std::string pingmsg = "JP2PING " + this->id();

        while (!m_p->m_destroymarker)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            if (m_p->m_agent)
            {
                if (juice_get_state(m_p->m_agent) > JUICE_STATE_DISCONNECTED)
                {
                    if (m_p->m_lastReceived + std::chrono::milliseconds(5000) < std::chrono::system_clock::now())
                    {
                        std::cout << "disconnected???" << std::endl;

                        // TODO disconnected? reconnect?
                        m_p->m_onStateChangedHandler(m_p->m_agent, JUICE_STATE_DISCONNECTED, this);

                        // TODO what else?
                        disconnect();
                    }
                }

                if (m_p->m_agent && juice_get_state(m_p->m_agent) >= JUICE_STATE_CONNECTED)
                {
                    // send ping, raw message
                    sendMessage(reinterpret_cast<const uint8_t *>(pingmsg.c_str()), pingmsg.size());
                }
            }
        }
    });
}

//******************************************************************************
//******************************************************************************
JuiceClient::~JuiceClient()
{
    if (m_p->m_pinger.joinable())
    {
        m_p->m_destroymarker = true;
        m_p->m_pinger.join();
    }
    disconnect();
}

//******************************************************************************
//******************************************************************************
// static
void JuiceClient::impl::onStateChanged(juice_agent_t * agent, juice_state_t state, void * user_ptr)
{
    // TODO ugly, use bind etc.
    dc::JuiceClient       * caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl * cimpl  = caller->m_p.get();

    cimpl->m_lastReceived = std::chrono::system_clock::now();

    // default, call handler
    if (caller->m_p->m_onStateChangedHandler)
    {
        caller->m_p->m_onStateChangedHandler(agent, state, user_ptr);
    }
}

//******************************************************************************
//******************************************************************************
// static
void JuiceClient::impl::onRecv(juice_agent_t * agent, const char * data, size_t size, void * user_ptr)
{
    // TODO ugly, use bind etc.
    dc::JuiceClient       * caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl * cimpl  = caller->m_p.get();

    cimpl->m_lastReceived = std::chrono::system_clock::now();

    // std::cout << size << " bytes from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    // TODO need incapsulate in client
    if (memcmp(data, "JP2PMSG ", 8) == 0)
    {
        // p2p text message, need forward to handler?
        std::cout << size << " bytes from " << caller->id() << std::endl;
        std::cout << "    " << std::string(data+8, size-8) << std::endl;
        return;
    }
    if (memcmp(data, "JP2PING ", 8) == 0)
    {
        // ping
//        std::cout << size << " bytes ping from " << caller->id() << std::endl;
//        std::cout << "    " << std::string(data, size) << std::endl;

        std::string pongmsg = "JP2PONG " + caller->id();
        caller->sendMessage(reinterpret_cast<const uint8_t *>(pongmsg.c_str()), pongmsg.size());
        return;
    }
    if (memcmp(data, "JP2PONG ", 8) == 0)
    {
        // pong
//        std::cout << size << " bytes pong from " << caller->id() << std::endl;
//        std::cout << "    " << std::string(data, size) << std::endl;
        return;
    }

    // default, call handler
    if (caller->m_p->m_onReceiveHandler)
    {
        caller->m_p->m_onReceiveHandler(agent, data, size, user_ptr);
    }
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::init()
{
    memset(&m_p->m_config, 0, sizeof(juice_config_t));

    m_p->m_config.stun_server_host = "62.210.189.244";
    m_p->m_config.stun_server_port = 3478;

    memset(&m_p->m_turnServer, 0, sizeof(juice_turn_server_t));

    m_p->m_turnServer.host     = "62.210.189.244";
    m_p->m_turnServer.port     = 3478;
    m_p->m_turnServer.username = "testUser";
    m_p->m_turnServer.password = "testPassword";

    m_p->m_config.turn_servers       = &m_p->m_turnServer;
    m_p->m_config.turn_servers_count = 1;

//    m_p->m_config.local_port_range_begin = 50050;
//    m_p->m_config.local_port_range_end   = 50050;

    m_p->m_config.cb_state_changed  = &impl::onStateChanged;
    m_p->m_config.cb_candidate      = m_p->m_onCandidateHandler;
    m_p->m_config.cb_gathering_done = m_p->m_onGatheringDoneHandler;
    m_p->m_config.cb_recv           = &impl::onRecv;
    m_p->m_config.user_ptr          = this;

    m_p->m_agent = juice_create(&m_p->m_config);
    return (m_p->m_agent != nullptr);
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::init(const std::string & sdp)
{
    return (init() &&
            juice_set_remote_description(m_p->m_agent, sdp.c_str()) == JUICE_ERR_SUCCESS &&
            juice_gather_candidates(m_p->m_agent) == JUICE_ERR_SUCCESS);
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::connect(const std::string & sdpRemote)
{
    if (!m_p->m_agent)
    {
        std::cout << "need init" << std::endl;
        return false;
    }
    return (juice_set_remote_description(m_p->m_agent, sdpRemote.c_str()) == JUICE_ERR_SUCCESS &&
            juice_gather_candidates(m_p->m_agent) == JUICE_ERR_SUCCESS);
}

//******************************************************************************
//******************************************************************************
void JuiceClient::disconnect()
{
    if (!m_p->m_agent)
    {
        return;
    }
    juice_destroy(m_p->m_agent);
    m_p->m_agent = nullptr;
}

//******************************************************************************
//******************************************************************************
void JuiceClient::addRemote(const std::string & sdpRemote)
{
    juice_add_remote_candidate(m_p->m_agent, sdpRemote.c_str());
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendMessage(const std::string & message)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    // text message with prefix

    const static size_t prefixsize = 8;
    const static size_t datasize   = 1096;
    char buffer[datasize] = "JP2PMSG ";
    const uint32_t size = std::min(message.size(), datasize);
    memcpy(buffer + 8, message.c_str(), size);
    juice_send(m_p->m_agent, buffer, size + prefixsize);
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendMessage(const uint8_t * data, const uint32_t size)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    // plain message

    juice_send(m_p->m_agent, reinterpret_cast<const char *>(data), size);
}

//******************************************************************************
//******************************************************************************
std::string JuiceClient::id() const
{
    return m_p->m_id;
}

//******************************************************************************
//******************************************************************************
std::string JuiceClient::sdp() const
{
    char sdp[JUICE_MAX_SDP_STRING_LEN];
    juice_get_local_description(m_p->m_agent, sdp, JUICE_MAX_SDP_STRING_LEN);
    // std::cout << "Local description " << sdp << std::endl;
    return sdp;
}

//******************************************************************************
//******************************************************************************
juice_state_t JuiceClient::state() const
{
    if (m_p->m_agent)
    {
        return juice_get_state(m_p->m_agent);
    }

    return JUICE_STATE_FAILED;
}

} // namespace dc
