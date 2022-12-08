//******************************************************************************
//******************************************************************************

#ifndef GRPCCLIENT_H
#define GRPCCLIENT_H

//******************************************************************************
//******************************************************************************

// TODO move to pimpl to not expose grpc headers outside
#include "autodiscovery.grpc.pb.h"

#include <atomic>
#include <list>
#include <sstream>
#include <thread>
#include <vector>

#include <grpcpp/grpcpp.h>

//******************************************************************************
//******************************************************************************
namespace dc
{

typedef std::function<void(const dc::Message &msg)> on_message_handler_t;
typedef std::function<void(const std::vector<dc::Peer> &list)> on_peers_handler_t;

class GRpcClient;
typedef std::shared_ptr<GRpcClient> GRpcClientPtr;

//******************************************************************************
//******************************************************************************
class GRpcClient
{
  public:
    GRpcClient(const std::string &id, const std::string &group, const dc::PeerType peerType,
               std::shared_ptr<grpc::Channel> channel)
        : m_stub(dc::DcRpc::NewStub(channel)), m_peerMutex(), m_messageHandler(nullptr)
    {
        m_peer.set_id(id);
        m_peer.set_group(group);
        m_peer.set_type(peerType);
    }

    ~GRpcClient()
    {
        if (m_reader.joinable())
        {
            unsubscribe();
        }
    }

  public:
    const Peer &peer() const
    {
        return m_peer;
    }

    void subscribe(PeerType type = READER, on_message_handler_t handler = nullptr);
    void updateType(PeerType newType);
    void updateGroup(const std::string& newGroup);
    void unsubscribe();
    void sendMessage(const std::string &to, const dc::MessageType type, const std::string &data);

    bool p2pInit(const std::string &to, const std::string &sdp);
    bool p2pAccept(const std::string &to, const std::string &sdp);
    bool p2pCandidate(const std::string &to, const std::string &candidate);
    bool p2pDisconnect(const std::string &to, const std::string &reason);

  protected:
    void updatePeer();

  private:
    std::unique_ptr<dc::DcRpc::Stub> m_stub;

    std::mutex m_peerMutex;
    dc::Peer m_peer;

    std::thread m_reader;
    std::thread m_peersReader;

    std::map<std::string, Peer> m_peers;

    on_message_handler_t m_messageHandler;
    on_peers_handler_t m_peersHandler;
};

} // namespace dc

#endif // GRPCCLIENT_H
