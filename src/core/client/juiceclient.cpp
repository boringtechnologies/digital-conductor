//******************************************************************************
//******************************************************************************

#include "core/juiceclient.h"

#include <atomic>
#include <cassert>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>

#include <fmt/core.h>
#include <juice/juice.h>

using namespace std::chrono_literals;

//******************************************************************************
//******************************************************************************
namespace dc
{

constexpr const std::string_view JP2P_PING  = "JP2PING ";
constexpr const std::string_view JP2P_PONG  = "JP2PONG ";
constexpr const std::string_view JP2P_MSG   = "JP2PMSG ";
constexpr const std::string_view JP2P_VIDEO = "JP2PVID ";
constexpr const std::string_view JP2P_AUDIO = "JP2PAUD ";
constexpr const size_t JP2P_HEADER_SIZE     = 8;

static_assert(JP2P_PING.size() == JP2P_HEADER_SIZE && JP2P_PONG.size() == JP2P_HEADER_SIZE &&
                  JP2P_MSG.size() == JP2P_HEADER_SIZE && JP2P_VIDEO.size() == JP2P_HEADER_SIZE &&
                  JP2P_AUDIO.size() == JP2P_HEADER_SIZE,
              "All P2P headers must have the same lenght");

//******************************************************************************
//******************************************************************************
class JuiceClient::impl
{
    friend class JuiceClient;

    impl(JuiceClient *owner, const std::string &id, on_state_changed_t onStateChangedHandler,
         on_candidate_t onCandidateHandler, on_gathering_done_t onGatheringDoneHandler, on_recv_t onVideoReceiveHandler,
         on_recv_t onAudioReceiveHandler)
        : m_o(owner), m_id(id), m_onStateChangedHandler(onStateChangedHandler),
          m_onCandidateHandler(onCandidateHandler), m_onGatheringDoneHandler(onGatheringDoneHandler),
          m_onVideoReceiveHandler(onVideoReceiveHandler), m_onAudioReceiveHandler(onAudioReceiveHandler),
          m_destroymarker(false)
    {
    }

    JuiceClient *m_o = nullptr;
    std::string m_id;
    juice_agent_t *m_agent = nullptr;
    juice_config_t m_config;
    juice_turn_server_t m_turnServer;

    on_state_changed_t m_onStateChangedHandler;
    on_candidate_t m_onCandidateHandler;
    on_gathering_done_t m_onGatheringDoneHandler;
    on_recv_t m_onVideoReceiveHandler;
    on_recv_t m_onAudioReceiveHandler;

    std::atomic<bool> m_destroymarker;
    std::mutex m_sendLocker;
    std::thread m_pinger;
    static size_t m_pingCounter;
    static size_t m_pongCounter;

    std::chrono::system_clock::time_point m_lastReceived;

    static std::chrono::milliseconds nowTimestamp();
    static std::chrono::milliseconds parseTimestamp(const char *payload);

    static void onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr);
    static void onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr);
    static void onGatheringDone(juice_agent_t *agent, void *user_ptr);
    static void onRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr);
};

//******************************************************************************
//******************************************************************************
JuiceClient::JuiceClient(const std::string &id, on_state_changed_t onStateChangedHandler,
                         on_candidate_t onCandidateHandler, on_gathering_done_t onGatheringDoneHandler,
                         on_recv_t onVideoReceiveHandler, on_recv_t onAudioReceiveHandler)
    : m_p(new impl(this, id, onStateChangedHandler, onCandidateHandler, onGatheringDoneHandler, onVideoReceiveHandler,
                   onAudioReceiveHandler))
{
    m_p->m_pinger = std::thread([this]() {
        while (!m_p->m_destroymarker)
        {
            std::this_thread::sleep_for(1s);

            if (m_p->m_agent)
            {
                if (m_p->m_agent && juice_get_state(m_p->m_agent) >= JUICE_STATE_CONNECTED)
                {
                    auto now               = m_p->nowTimestamp();
                    std::string latencymsg = fmt::format("{}{} {}", JP2P_PING, m_p->m_id, now.count());
                    sendMessage(reinterpret_cast<const uint8_t *>(latencymsg.c_str()), latencymsg.size());
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

size_t JuiceClient::impl::m_pingCounter = 0;
size_t JuiceClient::impl::m_pongCounter = 0;

std::chrono::milliseconds JuiceClient::impl::nowTimestamp()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

std::chrono::milliseconds JuiceClient::impl::parseTimestamp(const char *payload_cstr)
{
    try
    {
        std::string payload   = payload_cstr;
        std::string timestamp = payload.substr(payload.find_last_of(' ') + 1);
        return std::chrono::milliseconds(std::stol(timestamp.c_str())); // TODO convert to safe
    }
    catch (...)
    {
        // fmt::print("Cannot parse time from \"{}\" payload", payload_cstr);
    }
    return 0ms;
}

// static
void JuiceClient::impl::onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr)
{
    std::cout << "[juice] onStateChanged\n";

    // TODO ugly, use bind etc.
    dc::JuiceClient *caller      = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl *cimpl = caller->m_p.get();

    cimpl->m_lastReceived = std::chrono::system_clock::now();

    // default, call handler
    caller->m_p->m_onStateChangedHandler(agent, state, user_ptr);
}

//******************************************************************************
//******************************************************************************
// static
void JuiceClient::impl::onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr)
{
    std::cout << "[juice] onCandidate\n";

    // TODO ugly, use bind etc.
    dc::JuiceClient *caller      = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl *cimpl = caller->m_p.get();

    // default, call handler
    cimpl->m_onCandidateHandler(agent, sdp, user_ptr);
}

//******************************************************************************
//******************************************************************************
// static
void JuiceClient::impl::onGatheringDone(juice_agent_t *agent, void *user_ptr)
{
    std::cout << "[juice] onCandidate\n";

    // TODO ugly, use bind etc.
    dc::JuiceClient *caller      = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl *cimpl = caller->m_p.get();

    // default, call handler
    cimpl->m_onGatheringDoneHandler(agent, user_ptr);
}

//******************************************************************************
//******************************************************************************
// static
void JuiceClient::impl::onRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)
{
    // TODO ugly, use bind etc.
    dc::JuiceClient *caller      = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    dc::JuiceClient::impl *cimpl = caller->m_p.get();

    cimpl->m_lastReceived = std::chrono::system_clock::now();

    // std::cout << size << " bytes from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    // TODO need incapsulate in client
    if (memcmp(data, JP2P_MSG.data(), JP2P_HEADER_SIZE) == 0)
    {
        // p2p text message, need forward to handler?
        std::cout << size << " bytes from " << caller->id() << std::endl;
        std::cout << "    " << std::string(data + JP2P_HEADER_SIZE, size - JP2P_HEADER_SIZE) << std::endl;
    }
    else if (memcmp(data, JP2P_PING.data(), JP2P_HEADER_SIZE) == 0)
    {
        auto ping    = parseTimestamp(data);
        auto now     = nowTimestamp();
        auto latency = now - ping;

        if (m_pingCounter++ % 10 == 0 && ping.count() != 0 )
        {
            fmt::print("JP2P_PING from {} now:{}ms ping:{}ms latency{}ms\n", caller->id(), now.count(), ping.count(),
                       latency.count());
        }

        std::string pongmsg = fmt::format("{}{} {}", JP2P_PONG, caller->id(), now.count());
        caller->sendMessage(reinterpret_cast<const uint8_t *>(pongmsg.c_str()), pongmsg.size());
    }
    else if (memcmp(data, JP2P_PONG.data(), JP2P_HEADER_SIZE) == 0)
    {
        auto pong    = parseTimestamp(data);
        auto now     = nowTimestamp();
        auto latency = now - pong;

        if (m_pongCounter++ % 10 == 0 && pong.count() != 0)
        {
            fmt::print("JP2P_PONG from {} now:{}ms pong:{}ms latency{}ms\n", caller->id(), now.count(), pong.count(),
                       latency.count());
        }
    }
    else if (memcmp(data, JP2P_AUDIO.data(), JP2P_HEADER_SIZE) == 0)
    {
        caller->m_p->m_onAudioReceiveHandler(agent, data + JP2P_HEADER_SIZE, size - JP2P_HEADER_SIZE, user_ptr);
    }
    else if (memcmp(data, JP2P_VIDEO.data(), JP2P_HEADER_SIZE) == 0)
    {
        caller->m_p->m_onVideoReceiveHandler(agent, data + JP2P_HEADER_SIZE, size - JP2P_HEADER_SIZE, user_ptr);
    }
    else
    {
        std::cerr << "Unknow type of traffic\n";
        assert(false);
    }
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::init()
{
    constexpr const char *sturn_server_host  = "62.210.189.244";
    constexpr const size_t sturn_server_port = 3478;

    memset(&m_p->m_config, 0, sizeof(juice_config_t));

    m_p->m_config.stun_server_host = sturn_server_host;
    m_p->m_config.stun_server_port = sturn_server_port;

    memset(&m_p->m_turnServer, 0, sizeof(juice_turn_server_t));

    m_p->m_turnServer.host     = sturn_server_host;
    m_p->m_turnServer.port     = sturn_server_port;
    m_p->m_turnServer.username = "testUser";
    m_p->m_turnServer.password = "testPassword";

    m_p->m_config.turn_servers       = &m_p->m_turnServer;
    m_p->m_config.turn_servers_count = 1;

    //    m_p->m_config.local_port_range_begin = 50050;
    //    m_p->m_config.local_port_range_end   = 50050;

    m_p->m_config.cb_state_changed  = &impl::onStateChanged;
    m_p->m_config.cb_candidate      = &impl::onCandidate;
    m_p->m_config.cb_gathering_done = &impl::onGatheringDone;
    m_p->m_config.cb_recv           = &impl::onRecv;
    m_p->m_config.user_ptr          = this;

    m_p->m_agent = juice_create(&m_p->m_config);
    return (m_p->m_agent != nullptr);
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::init(const std::string &sdp)
{
    return (init() && juice_set_remote_description(m_p->m_agent, sdp.c_str()) == JUICE_ERR_SUCCESS &&
            juice_gather_candidates(m_p->m_agent) == JUICE_ERR_SUCCESS);
}

//******************************************************************************
//******************************************************************************
bool JuiceClient::connect(const std::string &sdpRemote)
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
void JuiceClient::addRemote(const std::string &sdpRemote)
{
    juice_add_remote_candidate(m_p->m_agent, sdpRemote.c_str());
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendMessage(const std::string &message)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    // text message with prefix

    const static size_t datasize = 1096;
    char buffer[datasize];
    memcpy(buffer, JP2P_MSG.data(), JP2P_HEADER_SIZE);
    const uint32_t size = std::min(message.size(), datasize);
    memcpy(buffer + JP2P_HEADER_SIZE, message.c_str(), size);
    juice_send(m_p->m_agent, buffer, size + JP2P_HEADER_SIZE);
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendMessage(const uint8_t *data, const uint32_t size)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    // plain message

    juice_send(m_p->m_agent, reinterpret_cast<const char *>(data), size);
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendVideo(const uint8_t *data, const uint32_t size)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    constexpr size_t buffer_size = 512000;
    if (size > buffer_size)
        return;

    static char buffer[buffer_size];
    memcpy(buffer, JP2P_VIDEO.data(), JP2P_HEADER_SIZE);
    memcpy(buffer + JP2P_HEADER_SIZE, data, size);

    // video payload
    juice_send(m_p->m_agent, const_cast<const char *>(buffer), size + JP2P_HEADER_SIZE);
}

//******************************************************************************
//******************************************************************************
void JuiceClient::sendAudio(const uint8_t *data, const uint32_t size)
{
    std::lock_guard<std::mutex> l(m_p->m_sendLocker);

    constexpr size_t buffer_size = 256000;
    if (size > buffer_size)
        return;

    static char buffer[buffer_size];
    memcpy(buffer, JP2P_AUDIO.data(), JP2P_HEADER_SIZE);
    memcpy(buffer + JP2P_HEADER_SIZE, data, size);

    // audio payload
    juice_send(m_p->m_agent, const_cast<const char *>(buffer), size + JP2P_HEADER_SIZE);
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
