//******************************************************************************
//******************************************************************************

#include "autodiscovery.grpc.pb.h"

#include <iostream>
#include <map>
#include <vector>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

//******************************************************************************
//******************************************************************************

namespace dc
{

typedef std::map<std::string, dc::Peer> peers_map_t;

class MessageWriter;

//******************************************************************************
//******************************************************************************
class Service : public dc::DcRpc::CallbackService
{
    grpc::ServerWriteReactor<dc::Message> *subscribe(grpc::CallbackServerContext *context,
                                                     const dc::Peer *request) override;

    grpc::ServerUnaryReactor *updatePeer(grpc::CallbackServerContext *context, const dc::Peer *request,
                                         dc::Empty *reply) override;

    grpc::ServerUnaryReactor *unsubscribe(grpc::CallbackServerContext *context, const dc::Peer *request,
                                          dc::Empty *reply) override;

    grpc::ServerUnaryReactor *sendMessage(grpc::CallbackServerContext *context, const dc::Message *request,
                                          dc::Empty *reply) override;

    grpc::ServerUnaryReactor *p2pInit(grpc::CallbackServerContext *context, const dc::P2PInit *request,
                                      dc::Empty *reply) override;

    grpc::ServerUnaryReactor *p2pAccept(grpc::CallbackServerContext *context, const dc::P2PAccept *request,
                                        dc::Empty *reply) override;

    grpc::ServerUnaryReactor *p2pCandidate(grpc::CallbackServerContext *context, const dc::P2PCandidate *request,
                                           dc::Empty *reply) override;

    grpc::ServerUnaryReactor *p2pDisconnect(grpc::CallbackServerContext *context, const dc::P2PDisconnect *request,
                                            dc::Empty *reply) override;

  public:
    // hack, need to review architecture
    void clientDisconnected(const std::string &id);

  private:
    void broadcastPeerList(const std::string &origin, const std::string &group, const std::string &skipId = "");

    grpc::ServerUnaryReactor *p2pAction(grpc::CallbackServerContext *context, const std::string &action,
                                        const std::string &fromId, const std::string &toId, MessageType messageType,
                                        const std::string &data);

    peers_map_t m_peers;

    typedef std::map<std::string, MessageWriter *> writers_t;
    writers_t m_writers;
};

//******************************************************************************
//******************************************************************************
class MessageWriter : public grpc::ServerWriteReactor<dc::Message>
{
  public:
    MessageWriter(Service *owner, const std::string &id) : m_service(owner), m_id(id)
    {
    }

  public:
    void OnDone() override
    {
        // std::cout << "OnDone()" << std::endl;
        delete this;
    }

    void OnWriteDone(bool /*ok*/) override
    {
        // std::cout << "OnWriteDone()" << std::endl;
    }

    void OnCancel() override
    {
        // std::cout << "OnCancel()" << std::endl;
        std::cout << "[messageWriter] " << m_id << " disconnected, unsubscribed" << std::endl;
        m_service->clientDisconnected(m_id);
        delete this;
    }

  private:
    Service *m_service;
    std::string m_id;
};

//******************************************************************************
//******************************************************************************
void Service::clientDisconnected(const std::string &id)
{
    m_peers.erase(id);
    m_writers.erase(id);
}

//******************************************************************************
//******************************************************************************
void Service::broadcastPeerList(const std::string &origin, const std::string &group, const std::string &skipId)
{
    // TODO synchronization?
    std::vector<dc::Peer> peers;
    for (const std::pair<std::string, dc::Peer> &pair : m_peers)
    {
        const dc::Peer &peer = pair.second;
        if (!peer.group().empty() && peer.group() == group)
        {
            peers.push_back(peer);
        }
    }

    PeerList peerList;
    *peerList.mutable_peers() = {peers.begin(), peers.end()};

    for (const auto &pair : m_peers)
    {
        const std::string &id = pair.first;
        const dc::Peer &peer  = pair.second;

        if (id == skipId)
        {
            std::cerr << "[broadcastPeerList] skipping for user[" << id << "]" << std::endl;
            continue;
        }

        if (peer.group() != group)
        {
            continue;
        }

        writers_t::iterator it = m_writers.find(id);
        if (it == m_writers.end())
        {
            std::cerr << "[broadcastPeerList] [bug] cannot find writer for user[" << id << "]" << std::endl;
            continue;
        }

        std::cout << "[broadcastPeerList] trigger[" << origin << "] for user[" << id << "]" << std::endl;

        // FIXME memory leak
        dc::Message *m = new dc::Message;
        m->set_from(origin);
        m->set_to(id);
        m->set_type(P2PLIST);
        m->mutable_peers()->CopyFrom(peerList);
        // END

        it->second->StartWrite(m);
    }
}

//******************************************************************************
//******************************************************************************
grpc::ServerWriteReactor<dc::Message> *Service::subscribe(grpc::CallbackServerContext *context, const dc::Peer *request)
{
    std::cout << "[subscribe] called from " << request->id() << " " << context->peer() << std::endl;

    peers_map_t::iterator it = m_peers.find(request->id());
    if (it != m_peers.end())
    {
        std::cout << "    user exists" << std::endl;

        MessageWriter *reactor = new MessageWriter(this, request->id());
        reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "User exists"));
        return reactor;
    }

    MessageWriter *w = new MessageWriter(this, request->id());

    m_peers[request->id()]   = *request;
    m_writers[request->id()] = w;

    broadcastPeerList(request->id(), request->group());

    return w;
}

grpc::ServerUnaryReactor *Service::updatePeer(grpc::CallbackServerContext *context, const dc::Peer *request,
                                              dc::Empty * /*reply*/)
{
    std::cout << "[updatePeer] called by " << request->id() << std::endl;

    grpc::ServerUnaryReactor *reactor = context->DefaultReactor();

    if (m_peers.find(request->id()) == m_peers.end())
    {
        std::cout << "[updatePeer] user not found" << std::endl;

        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found"));
        return reactor;
    }

    const std::string oldGroup = m_peers[request->id()].group();
    const dc::PeerType oldType = m_peers[request->id()].type();

    m_peers[request->id()] = *request;

    if (oldType != request->type()) // update type
    {
        broadcastPeerList(request->id(), request->group(), request->id());
    }
    else if (oldGroup != request->group()) // notify old group & new group
    {
        broadcastPeerList(request->id(), request->group());
        broadcastPeerList(request->id(), oldGroup);
    }
    else
    {
        std::cout << "[updatePeer] skip update: no changes of type or group" << std::endl;
    }

    reactor->Finish(grpc::Status::OK);

    std::cout << "[updatePeer] succeed for " << request->id() << std::endl;

    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::unsubscribe(grpc::CallbackServerContext *context, const dc::Peer *request,
                                               dc::Empty * /*reply*/)
{
    std::cout << "[unsubscribe] called by " << request->id() << std::endl;

    grpc::ServerUnaryReactor *reactor = context->DefaultReactor();

    if (m_peers.find(request->id()) == m_peers.end())
    {
        std::cout << "[unsubscribe] user not found" << std::endl;

        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "User not found"));
        return reactor;
    }

    MessageWriter *mw = m_writers.at(request->id());
    mw->Finish(grpc::Status::OK);

    m_peers.erase(request->id());
    m_writers.erase(request->id());

    broadcastPeerList(request->id(), request->group());

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::sendMessage(grpc::CallbackServerContext *context, const dc::Message *request,
                                               dc::Empty * /*reply*/)
{
    const auto &fromId = request->from();
    const auto &toId   = request->to();

    std::cout << "[sendMessage] from " << fromId << " to " << toId << std::endl;

    grpc::ServerUnaryReactor *reactor = context->DefaultReactor();

    peers_map_t::iterator from = m_peers.find(fromId);
    if (from == m_peers.end())
    {
        std::cerr << "[sendMessage] no sender found for " << fromId << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Sender not found"));
        return reactor;
    }

    peers_map_t::iterator to = m_peers.find(request->to());
    if (to == m_peers.end())
    {
        // TODO find and broadcast to group?
        std::cerr << "[sendMessage] no receiver found for" << toId << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Receiver not found"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        std::cerr << "[sendMessage] [bug] no receiver writer found for " << toId << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "No writer"));
        return reactor;
    }

    it->second->StartWrite(request);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::p2pInit(grpc::CallbackServerContext *context, const dc::P2PInit *request,
                                           dc::Empty * /*reply*/)
{
    return p2pAction(context, __func__, request->from(), request->to(), P2PINIT, request->sdp());
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::p2pAccept(grpc::CallbackServerContext *context, const dc::P2PAccept *request,
                                             dc::Empty * /*reply*/)
{
    return p2pAction(context, __func__, request->from(), request->to(), P2PACCEPT, request->sdp());
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::p2pCandidate(grpc::CallbackServerContext *context, const dc::P2PCandidate *request,
                                                dc::Empty * /*reply*/)
{
    return p2pAction(context, __func__, request->from(), request->to(), P2PCANDIDATE, request->candidate());
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor *Service::p2pDisconnect(grpc::CallbackServerContext *context, const dc::P2PDisconnect *request,
                                                 dc::Empty * /*reply*/)
{
    return p2pAction(context, __func__, request->from(), request->to(), P2PDISCONNECT, request->reason());
}

grpc::ServerUnaryReactor *Service::p2pAction(grpc::CallbackServerContext *context, const std::string &action,
                                             const std::string &fromId, const std::string &toId,
                                             MessageType messageType, const std::string &data)
{
    std::cout << "[p2p" << action << "] from " << fromId << " to " << toId << std::endl;

    grpc::ServerUnaryReactor *reactor = context->DefaultReactor();

    if (fromId == toId)
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Sender eq receiver"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(toId);
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Peer not found"));
        return reactor;
    }

    dc::Message m;
    m.set_from(fromId);
    m.set_to(toId);
    m.set_type(messageType);
    m.set_data(data);
    it->second->StartWrite(&m);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

} // namespace dc

//******************************************************************************
//******************************************************************************
int main()
{
    char *portFromEnv = std::getenv("DC_GRPC_SERVER_PORT");
    std::string port  = portFromEnv ? portFromEnv : "50051";

    dc::Service service;

    grpc::ServerBuilder builder;
    //    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
    //    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
    //    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    builder.RegisterService(&service);

    builder.AddListeningPort("0.0.0.0:" + port, grpc::InsecureServerCredentials());

    std::cout << "Server started on 0.0.0.0:" << port << " (rev: " << DC_GIT_REV << ")" << std::endl;

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();

    return 0;
}
