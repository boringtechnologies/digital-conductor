//******************************************************************************
//******************************************************************************

#ifndef JUICECLIENT_H
#define JUICECLIENT_H

#include <memory>

#include <juice/juice.h>

//******************************************************************************
//******************************************************************************
namespace rpcroc
{

typedef void (* on_state_changed_t)(juice_agent_t * agent, juice_state_t state, void * user_ptr);
typedef void (* on_candidate_t)(juice_agent_t * agent, const char * sdp, void * user_ptr);
typedef void (* on_gathering_done_t)(juice_agent_t * agent, void * user_ptr);
typedef void (* on_recv_t)(juice_agent_t * agent, const char * data, size_t size, void * user_ptr);

//******************************************************************************
//******************************************************************************
class JuiceClient
{
public:
    JuiceClient(const std::string & id,
                on_state_changed_t onStateChangedHandler, on_candidate_t onCandidateHandler,
                on_gathering_done_t onGatheringDoneHandler, on_recv_t onReceiveHandler);
    ~JuiceClient();

public:
    bool init();
    bool init(const std::string & sdp);
    bool connect(const std::string & sdpRemote);
    void disconnect();
    void addRemote(const std::string & sdpRemote);
    void sendMessage(const std::string & message);
    void sendMessage(const uint8_t * data, const uint32_t size);

    std::string id() const;
    std::string sdp() const;

    juice_state_t state() const;

private:
    class impl;
    std::shared_ptr<impl> m_p;
};

} // namespace rpcroc

#endif // JUICECLIENT_H
