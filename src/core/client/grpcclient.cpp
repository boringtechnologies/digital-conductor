//******************************************************************************
//******************************************************************************

#include "grpcclient.h"

//******************************************************************************
//******************************************************************************
namespace dc
{

//******************************************************************************
//******************************************************************************
std::vector<Peer> GRpcClient::getPeerList()
{
    std::cout << "getPeerList()" << std::endl;

    std::vector<Peer> result;

    // TODO check last get peer time and request from server if need
    if (m_lastPeersRequested + std::chrono::milliseconds(5000) < std::chrono::system_clock::now())
    {
        m_peers.clear();

        GetPeerList request;
        request.set_id(m_id);

        grpc::ClientContext context;

        std::unique_ptr<grpc::ClientReader<Peer> > reader(m_stub->getPeerList(&context, request));

        Peer peer;
        std::cout << "Received peer list:" << std::endl;
        while (reader->Read(&peer))
        {
            std::cout << "    <" << peer.id() << "@" << peer.group()
                      << "> type " << peer.type() << std::endl;
                      // << " sdp (" << peer.sdp() << ")" << std::endl;

            m_peers[peer.id()] = peer;
        }

        grpc::Status status = reader->Finish();
        if (!status.ok())
        {
            std::cout << "getPeerList finished with error "
                      << status.error_code() << ": " << status.error_message() << std::endl;
            return result;
        }

        m_lastPeersRequested = std::chrono::system_clock::now();
    }

    for (const std::pair<std::string, Peer> & p : m_peers)
    {
        result.emplace_back(p.second);
    }

    return result;
}

//******************************************************************************
//******************************************************************************
Peer GRpcClient::getPeer(const std::string & id) const
{
    if (m_peers.find(id) == m_peers.end())
    {
        return Peer();
    }

    return m_peers.at(id);
}

//******************************************************************************
//******************************************************************************
void GRpcClient::subscribe(const uint32_t type, on_message_handler_t handler)
{
    std::cout << "subscribe()" << std::endl;

    if (m_reader.joinable())
    {
        std::cout << "already subscribed" << std::endl;
        return;
    }

    m_messageHandler = handler;

    m_reader = std::thread([this, type]()
    {
        Peer peer;
        peer.set_id(m_id);
        peer.set_group(m_group);
        peer.set_type(type == 2 ? WRITER :
                      type == 3 ? READER | WRITER :
                                  READER);

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<Message> > reader(m_stub->subscribe(&context, peer));

        Message m;
        while (reader->Read(&m))
        {
            // std::cout << "Message from " << m.from() << " --> " << m.msg() << std::endl;

            if (m_messageHandler)
            {
                m_messageHandler(m.from(), m.to(), m.msg());
            }
        }

        grpc::Status status = reader->Finish();
        if (!status.ok())
        {
            std::cout << "subscribe error "
                      << status.error_code() << ": " << status.error_message() << std::endl;
        }
        m_messageHandler = nullptr;
        std::cout << "subscribe finished" << std::endl;
    });
}

//******************************************************************************
//******************************************************************************
void GRpcClient::unsubscribe()
{
    std::cout << "unsubscribe()" << std::endl;

    if (!m_reader.joinable())
    {
        std::cout << "already unsubscribed" << std::endl;
        return;
    }

    Peer peer;
    peer.set_id(m_id);
    peer.set_group(m_group);
    peer.set_type(READER | WRITER);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->unsubscribe(&context, peer, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return;
    }

    if (m_reader.joinable())
    {
        m_reader.join();
    }
}

//******************************************************************************
//******************************************************************************
void GRpcClient::sendMessage(const std::string & to, const std::string & msg)
{
    Message m;
    m.set_from(m_id);
    m.set_to(to);
    m.set_msg(msg);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->sendMessage(&context, m, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return;
    }
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pInit(const std::string & to, const std::string & sdp)
{
    P2PInit initmsg;
    initmsg.set_from(m_id);
    initmsg.set_to(to);
    initmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pInit(&context, initmsg, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pAccept(const std::string & to, const std::string & sdp)
{
    P2PAccept accmsg;
    accmsg.set_from(m_id);
    accmsg.set_to(to);
    accmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pAccept(&context, accmsg, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pCandidate(const std::string & to, const std::string & candidate)
{
    P2PCandidate cndmsg;
    cndmsg.set_from(m_id);
    cndmsg.set_to(to);
    cndmsg.set_candidate(candidate);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pCandidate(&context, cndmsg, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pDisconnect(const std::string & to, const std::string & reason)
{
    P2PDisconnect dmsg;
    dmsg.set_from(m_id);
    dmsg.set_to(to);
    dmsg.set_reason(reason);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pDisconnect(&context, dmsg, &resp);
    if (!status.ok())
    {
        std::cout << status.error_code() << " : " << status.error_message()
                  << std::endl;
        return false;
    }

    return true;
}

} // namespace dc
