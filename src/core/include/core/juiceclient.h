//******************************************************************************
//******************************************************************************

#ifndef JUICECLIENT_H
#define JUICECLIENT_H

#include <functional>
#include <memory>
#include <thread>

#include <juice/juice.h>

//******************************************************************************
//******************************************************************************
namespace dc
{

typedef std::chrono::duration<double, std::milli> double_millis_t;

typedef std::function<void(juice_agent_t *agent, juice_state_t state, void *user_ptr)> on_state_changed_t;
typedef std::function<void(juice_agent_t *agent, const char *sdp, void *user_ptr)> on_candidate_t;
typedef std::function<void(juice_agent_t *agent, void *user_ptr)> on_gathering_done_t;
typedef std::function<void(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)> on_recv_t;
typedef std::function<void(juice_agent_t *agent, const double_millis_t, void *user_ptr)> on_latency_t;

class JuiceClient;
typedef std::shared_ptr<JuiceClient> JuiceClientPtr;

//******************************************************************************
//******************************************************************************
class JuiceClient
{
  public:
    JuiceClient(const std::string &id, on_state_changed_t onStateChangedHandler, on_candidate_t onCandidateHandler,
                on_gathering_done_t onGatheringDoneHandler, on_recv_t onVideoReceiveHandler,
                on_recv_t onAudioReceiveHandler, on_latency_t onLatencyReceiveHandler);
    ~JuiceClient();

  public:
    bool init();
    bool init(const std::string &sdp);
    bool connect(const std::string &sdpRemote);
    void disconnect();
    void addRemote(const std::string &sdpRemote);
    void sendMessage(const std::string &message);
    void sendMessage(const uint8_t *data, const uint32_t size);
    void sendVideo(const uint8_t *data, const uint32_t size);
    void sendAudio(const uint8_t *data, const uint32_t size);
    void sendLatency(const double_millis_t latency);

    std::string id() const;
    std::string sdp() const;

    juice_state_t state() const;

    double_millis_t averagePing() const;

  private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace dc

#endif // JUICECLIENT_H
