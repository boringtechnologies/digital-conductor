//******************************************************************************
//******************************************************************************

#ifndef GRPCCLIENT_H
#define GRPCCLIENT_H

//******************************************************************************
//******************************************************************************

#include "autodiscovery.grpc.pb.h"

#include <thread>
#include <list>
#include <atomic>
#include <sstream>

#include <grpcpp/grpcpp.h>

//******************************************************************************
//******************************************************************************
namespace dc
{

typedef std::function<void(const std::string & from, const std::string & to, const std::string & msg)> on_message_handler_t;

//******************************************************************************
//******************************************************************************
class GRpcClient
{
public:
    GRpcClient(const std::string & id, const std::string & group,
               std::shared_ptr<grpc::Channel> channel)
        : m_id(id)
        , m_group(group)
        , m_stub(dc::DcRpc::NewStub(channel))
        , m_messageHandler(nullptr)
        , m_lastPeersRequested(std::chrono::system_clock::now() - std::chrono::seconds(60))
    {}

    ~GRpcClient()
    {
        if (m_reader.joinable())
        {
            unsubscribe();
        }
    }

public:
    std::string id()    const { return m_id; }
    std::string group() const { return m_group; }

    Peer        getPeer(const std::string & id) const;

    std::vector<Peer> getPeerList();
    void subscribe(const uint32_t type = 1, on_message_handler_t handler = nullptr);
    void unsubscribe();
    void sendMessage(const std::string & to, const std::string & msg);

    bool p2pInit(const std::string & to, const std::string & sdp);
    bool p2pAccept(const std::string & to, const std::string & sdp);
    bool p2pCandidate(const std::string & to, const std::string & candidate);
    bool p2pDisconnect(const std::string & to, const std::string & reason);

private:
    std::unique_ptr<dc::DcRpc::Stub> m_stub;

    std::string m_id;
    std::string m_group;

    std::thread m_reader;

    std::chrono::system_clock::time_point  m_lastPeersRequested;
    std::map<std::string, Peer>            m_peers;

    on_message_handler_t m_messageHandler;
};

} // namespace dc

#endif // GRPCCLIENT_H
