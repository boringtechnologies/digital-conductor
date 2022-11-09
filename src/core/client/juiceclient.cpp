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

typedef std::chrono::duration<double, std::milli> double_millis_t;

static double_millis_t now_since_epoch()
{
    return std::chrono::duration_cast<double_millis_t>(std::chrono::system_clock::now().time_since_epoch());
}

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
          m_destroymarker(false), m_pingSeq(0), m_pingAvg(0ms), m_lastReceived()
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
    std::size_t m_pingSeq;
    double_millis_t m_pingAvg;

    std::chrono::system_clock::time_point m_lastReceived;

    void updatePingStat(const std::string &pongData);

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
                    std::string latencymsg = fmt::format("{}{} {}", JP2P_PING, m_p->m_id, now_since_epoch().count());
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
void JuiceClient::impl::updatePingStat(const std::string &pongData)
{
    try
    {
        std::string millis = pongData.substr(pongData.find_last_of(' ') + 1);

        auto latency =
            std::chrono::duration_cast<double_millis_t>(now_since_epoch() - double_millis_t(std::stol(millis.c_str())));

        m_pingSeq++;

        m_pingAvg = std::chrono::duration_cast<double_millis_t>(m_pingAvg + (latency - m_pingAvg) / (double)m_pingSeq);

        fmt::print("{} bytes from {}: seq={} time={:.2f} ms avg={:.2f} ms\n", pongData.size(), m_id, m_pingSeq,
                   latency.count(), m_pingAvg.count());
    }
    catch (...)
    {
        fmt::print("{} bytes from {}: cannot parse ping time from payload: {}", pongData.size(), m_id, pongData);
    }
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

    // fmt::print("[juice] {} bytes of {} from {} recieved\n", size, std::string(data, JP2P_HEADER_SIZE - 1),
    // caller->id());

    // TODO need incapsulate in client
    if (memcmp(data, JP2P_MSG.data(), JP2P_HEADER_SIZE) == 0)
    {
        // p2p text message, need forward to handler?
        fmt::print("[juice]\tfrom {} -> '{}'\n", caller->id(), data + JP2P_HEADER_SIZE);
    }
    else if (memcmp(data, JP2P_PING.data(), JP2P_HEADER_SIZE) == 0)
    {
        auto pongData       = std::string(data, size);
        auto timestamp      = pongData.substr(pongData.find_last_of(' ') + 1);
        std::string pongmsg = fmt::format("{}{} {}", JP2P_PONG, caller->id(), timestamp);
        caller->sendMessage(reinterpret_cast<const uint8_t *>(pongmsg.c_str()), pongmsg.size());
    }
    else if (memcmp(data, JP2P_PONG.data(), JP2P_HEADER_SIZE) == 0)
    {
        cimpl->updatePingStat(std::string(data, size));
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
        fmt::print("[juice] init {}", sdpRemote);
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
