//******************************************************************************
//******************************************************************************

//#include "absl/flags/flag.h"
//#include "absl/flags/parse.h"
//#include "absl/strings/str_cat.h"

#include "autodiscovery.grpc.pb.h"

#include <iostream>
#include <vector>
#include <map>

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/server_builder.h>

//******************************************************************************
//******************************************************************************
// ABSL_FLAG(int32_t, port, 50051, "Server port for service.");

namespace rpcroc
{

typedef std::map<std::string, rpcroc::Peer> peers_map_t;

class PeersWriter;
class MessageWriter;

//******************************************************************************
//******************************************************************************
class Service : public rpcroc::RpcRoc::CallbackService
{
    grpc::ServerWriteReactor<rpcroc::Peer> * getPeerList(grpc::CallbackServerContext * context,
                                                         const rpcroc::GetPeerList * request) override;

    grpc::ServerWriteReactor<rpcroc::Message> * subscribe(grpc::CallbackServerContext * context,
                                                          const rpcroc::Peer * request) override;

    grpc::ServerUnaryReactor * unsubscribe(grpc::CallbackServerContext * context,
                                           const rpcroc::Peer * request,
                                           rpcroc::Empty * reply) override;

    grpc::ServerUnaryReactor * sendMessage(grpc::CallbackServerContext * context,
                                           const rpcroc::Message * request,
                                           rpcroc::Empty * reply) override;

    grpc::ServerUnaryReactor * p2pInit(grpc::CallbackServerContext * context,
                                       const rpcroc::P2PInit * request,
                                       rpcroc::Empty * reply) override;

    grpc::ServerUnaryReactor * p2pAccept(grpc::CallbackServerContext * context,
                                         const rpcroc::P2PAccept * request,
                                         rpcroc::Empty * reply) override;

    grpc::ServerUnaryReactor * p2pCandidate(grpc::CallbackServerContext * context,
                                            const rpcroc::P2PCandidate * request,
                                            rpcroc::Empty * reply) override;

    grpc::ServerUnaryReactor * p2pDisconnect(grpc::CallbackServerContext * context,
                                             const rpcroc::P2PDisconnect * request,
                                             rpcroc::Empty * reply) override;

public:
    // hack, need to review architecture
    void clientDisconnected(const std::string & id);

private:
    peers_map_t m_peers;

    typedef std::map<std::string, MessageWriter * > writers_t;
    writers_t m_writers;

};

//******************************************************************************
//******************************************************************************
class PeersWriter : public grpc::ServerWriteReactor<rpcroc::Peer>
{
public:
    PeersWriter(const peers_map_t & pmap)
        : m_pmap(pmap)
        , m_pmapIt(m_pmap.begin())
    {
        nextWrite();
    }

public:
    void OnDone() override
    {
        delete this;
    }

    void OnWriteDone(bool /*ok*/) override
    {
        nextWrite();
    }

private:
    void nextWrite()
    {
        if (m_pmapIt == m_pmap.end())
        {
            Finish(grpc::Status::OK);
            return;
        }

        StartWrite(&(m_pmapIt++)->second);
    }

private:
    const peers_map_t           m_pmap;
    peers_map_t::const_iterator m_pmapIt;
};

//******************************************************************************
//******************************************************************************
grpc::ServerWriteReactor<rpcroc::Peer> *
            Service::getPeerList(grpc::CallbackServerContext * context,
                                 const rpcroc::GetPeerList * request)
{
    std::cout << "getPeerList called from " << request->id() << std::endl;
    std::cout << "    " << m_peers.size() << " peers" << std::endl;

    // take a copy of peers map and send to client
    return new PeersWriter(m_peers);
}

//******************************************************************************
//******************************************************************************
class MessageWriter : public grpc::ServerWriteReactor<rpcroc::Message>
{
public:
    MessageWriter(Service * owner, const std::string & id)
        : m_service(owner)
        , m_id(id)
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
        std::cout << m_id << " disconnected, unsubscribed" << std::endl;
        m_service->clientDisconnected(m_id);
        delete this;
    }

private:
    Service * m_service;
    std::string m_id;
};

//******************************************************************************
//******************************************************************************
void Service::clientDisconnected(const std::string & id)
{
    m_peers.erase(id);
    m_writers.erase(id);
}

//******************************************************************************
//******************************************************************************
grpc::ServerWriteReactor<rpcroc::Message> *
            Service::subscribe(grpc::CallbackServerContext * context,
                               const rpcroc::Peer * request)
{
    std::cout << "subscribe called from " << request->id() << " " << context->peer() << std::endl;

    peers_map_t::iterator it = m_peers.find(request->id());
    if (it != m_peers.end())
    {
        std::cout << "    user exists" << std::endl;

        MessageWriter * reactor = new MessageWriter(this, request->id());
        reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "User exists"));
        return reactor;
    }

    MessageWriter * w = new MessageWriter(this, request->id());

    m_peers[request->id()]          = *request;
    m_writers[request->id()]        = w;

    return w;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::unsubscribe(grpc::CallbackServerContext * context,
                                                const rpcroc::Peer * request,
                                                rpcroc::Empty * reply)
{
    std::cout << "unsubscribe called from " << request->id() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    if (m_peers.find(request->id()) == m_peers.end())
    {
        std::cout << "    user not found" << std::endl;

        reactor->Finish(grpc::Status(grpc::StatusCode::ALREADY_EXISTS, "User exists"));
        return reactor;
    }

    MessageWriter * mw = m_writers.at(request->id());
    mw->Finish(grpc::Status::OK);

    m_peers.erase(request->id());
    m_writers.erase(request->id());

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::sendMessage(grpc::CallbackServerContext * context,
                                                const rpcroc::Message * request,
                                                rpcroc::Empty * reply)
{
    std::cout << "message from " << request->from()
              << " to " << request->to() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    peers_map_t::iterator from = m_peers.find(request->from());
    if (from == m_peers.end())
    {
        std::cout << "    no sender" << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Sender not found"));
        return reactor;
    }
//    if ((from->second.type() & WRITER) == 0)
//    {
//        std::cout << "    sender not writer" << std::endl;
//        reactor->Finish(grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Sender not writer"));
//        return reactor;
//    }

    peers_map_t::iterator to = m_peers.find(request->to());
    if (to == m_peers.end())
    {
        // TODO find and broadcast to group?
        std::cout << "    no receiver" << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Receiver not found"));
        return reactor;
    }
//    if ((to->second.type() & READER) == 0)
//    {
//        std::cout << "    receiver not reader" << std::endl;
//        reactor->Finish(grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "Receiver not reader"));
//        return reactor;
//    }

    peers_map_t::iterator ito = m_peers.find(request->to());
    if (ito == m_peers.end())
    {
        // TODO find and broadcast to group?
        std::cout << "    no receiver" << std::endl;
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Receiver not found"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "No writer"));
        return reactor;
    }

    it->second->StartWrite(request);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::p2pInit(grpc::CallbackServerContext * context,
                                            const rpcroc::P2PInit * request,
                                            rpcroc::Empty * reply)
{
    std::cout << "p2pInit from " << request->from()
              << " to " << request->to() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    if (request->from() == request->to())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Sender eq receiver"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Peer not found"));
        return reactor;
    }

    rpcroc::Message m;
    m.set_from(request->from());
    m.set_to(request->to());
    m.set_msg("JP2PI " + request->sdp());
    it->second->StartWrite(&m);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::p2pAccept(grpc::CallbackServerContext * context,
                                              const rpcroc::P2PAccept * request,
                                              rpcroc::Empty * reply)
{
    std::cout << "p2pAccept from " << request->from()
              << " to " << request->to() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    if (request->from() == request->to())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Sender eq receiver"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Peer not found"));
        return reactor;
    }

    rpcroc::Message m;
    m.set_from(request->from());
    m.set_to(request->to());
    m.set_msg("JP2PA " + request->sdp());
    it->second->StartWrite(&m);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::p2pCandidate(grpc::CallbackServerContext * context,
                                                 const rpcroc::P2PCandidate * request,
                                                 rpcroc::Empty * reply)
{
    std::cout << "p2pCandidate from " << request->from()
              << " to " << request->to() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    if (request->from() == request->to())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Sender eq receiver"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Peer not found"));
        return reactor;
    }

    rpcroc::Message m;
    m.set_from(request->from());
    m.set_to(request->to());
    m.set_msg("JP2PC " + request->candidate());
    it->second->StartWrite(&m);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

//******************************************************************************
//******************************************************************************
grpc::ServerUnaryReactor * Service::p2pDisconnect(grpc::CallbackServerContext * context,
                                                  const rpcroc::P2PDisconnect * request,
                                                  rpcroc::Empty * reply)
{
    std::cout << "p2pDisconnect from " << request->from()
              << " to " << request->to() << std::endl;

    grpc::ServerUnaryReactor * reactor = context->DefaultReactor();

    if (request->from() == request->to())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::CANCELLED, "Sender eq receiver"));
        return reactor;
    }

    writers_t::iterator it = m_writers.find(request->to());
    if (it == m_writers.end())
    {
        reactor->Finish(grpc::Status(grpc::StatusCode::NOT_FOUND, "Peer not found"));
        return reactor;
    }

    rpcroc::Message m;
    m.set_from(request->from());
    m.set_to(request->to());
    m.set_msg("JP2PD " + request->reason());
    it->second->StartWrite(&m);

    reactor->Finish(grpc::Status::OK);
    return reactor;
}

} // namespace rpcroc

//******************************************************************************
//******************************************************************************
int main()
{
    uint32_t port = 50051; // absl::GetFlag(FLAGS_port);

    rpcroc::Service service;

    grpc::ServerBuilder builder;
//    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 2000);
//    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
//    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
    builder.RegisterService(&service);

    builder.AddListeningPort("0.0.0.0:" + std::to_string(port),
                             grpc::InsecureServerCredentials());

    std::cout << "Server started on 0.0.0.0:" << port << std::endl;

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();

    return 0;
}
