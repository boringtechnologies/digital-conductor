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
namespace rpcroc
{

typedef void (* on_message_handler_t)(const std::string & from, const std::string & to, const std::string & msg);

//******************************************************************************
//******************************************************************************
class GRpcClient
{
public:
    GRpcClient(const std::string & id, const std::string & group,
               std::shared_ptr<grpc::Channel> channel)
        : m_id(id)
        , m_group(group)
        , m_stub(rpcroc::RpcRoc::NewStub(channel))
        , m_messageHandler(nullptr)
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

    void getPeerList();
    void subscribe(const uint32_t type = 1, on_message_handler_t handler = nullptr);
    void unsubscribe();
    void sendMessage(const std::string & to, const std::string & msg);

    bool p2pInit(const std::string & to, const std::string & sdp);
    bool p2pAccept(const std::string & to, const std::string & sdp);
    bool p2pCandidate(const std::string & to, const std::string & candidate);
    bool p2pDisconnect(const std::string & to, const std::string & reason);

private:
    std::unique_ptr<rpcroc::RpcRoc::Stub> m_stub;

    std::string m_id;
    std::string m_group;

    std::thread m_reader;

    std::map<std::string, Peer> m_peers;

    on_message_handler_t m_messageHandler;
};

} // namespace rpcroc

#endif // GRPCCLIENT_H
