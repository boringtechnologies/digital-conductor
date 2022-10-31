//******************************************************************************
//******************************************************************************

#include "core/grpcclient.h"

//******************************************************************************
//******************************************************************************
namespace dc
{

//******************************************************************************
//******************************************************************************
Peer GRpcClient::getPeer(const std::string &id)
{
    std::lock_guard lock(m_peerMutex);

    if (m_peers.find(id) == m_peers.end())
    {
        return Peer();
    }

    return m_peers.at(id);
}

//******************************************************************************
//******************************************************************************
void GRpcClient::subscribe(PeerType type, on_message_handler_t handler)
{
    std::cout << "[grpc] "
              << "subscribe()" << std::endl;

    if (m_reader.joinable())
    {
        std::cout << "[grpc] "
                  << "already subscribed" << std::endl;
        return;
    }

    m_messageHandler = handler;

    m_reader = std::thread([this, type]() {
        m_peer.set_type(type);

        grpc::ClientContext context;
        std::unique_ptr<grpc::ClientReader<Message>> reader(m_stub->subscribe(&context, m_peer));

        Message m;
        while (reader->Read(&m))
        {
            // std::cout << "Message from " << m.from() << " --> " << m.msg() << std::endl;

            if (m_messageHandler)
            {
                m_messageHandler(m);
            }
        }

        grpc::Status status = reader->Finish();
        if (!status.ok())
        {
            std::cout << "[grpc] "
                      << "subscribe error " << status.error_code() << ": " << status.error_message() << std::endl;
        }
        m_messageHandler = nullptr;
        std::cout << "[grpc] "
                  << "subscribe finished" << std::endl;
    });
}

//******************************************************************************
//******************************************************************************
void GRpcClient::updatePeer(PeerType newType)
{
    std::cout << "[grpc] "
              << "updatePeer()" << std::endl;

    m_peer.set_type(newType);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->updatePeer(&context, m_peer, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] unsubscribe()" << status.error_code() << " : " << status.error_message() << std::endl;
        return;
    }
}

//******************************************************************************
//******************************************************************************
void GRpcClient::unsubscribe()
{
    std::cout << "[grpc] unsubscribe()" << std::endl;

    if (!m_reader.joinable())
    {
        std::cout << "[grpc] already unsubscribed" << std::endl;
        return;
    }

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->unsubscribe(&context, m_peer, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] unsubscribe()" << status.error_code() << " : " << status.error_message() << std::endl;
        return;
    }

    if (m_reader.joinable())
    {
        m_reader.join();
    }
}

//******************************************************************************
//******************************************************************************
void GRpcClient::sendMessage(const std::string &to, const dc::MessageType type, const std::string &data)
{
    Message m;
    m.set_from(m_peer.id());
    m.set_to(to);
    m.set_type(type);
    m.set_data(data);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->sendMessage(&context, m, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] " << status.error_code() << " : " << status.error_message() << std::endl;
        return;
    }
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pInit(const std::string &to, const std::string &sdp)
{
    P2PInit initmsg;
    initmsg.set_from(m_peer.id());
    initmsg.set_to(to);
    initmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pInit(&context, initmsg, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] " << status.error_code() << " : " << status.error_message() << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pAccept(const std::string &to, const std::string &sdp)
{
    std::cout << "[grpc] p2pAccept to:" << to << "\n";

    P2PAccept accmsg;
    accmsg.set_from(m_peer.id());
    accmsg.set_to(to);
    accmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pAccept(&context, accmsg, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] " << status.error_code() << " : " << status.error_message() << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pCandidate(const std::string &to, const std::string &candidate)
{
    std::cout << "[grpc] p2pCandidate\n";

    P2PCandidate cndmsg;
    cndmsg.set_from(m_peer.id());
    cndmsg.set_to(to);
    cndmsg.set_candidate(candidate);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pCandidate(&context, cndmsg, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] " << status.error_code() << " : " << status.error_message() << std::endl;
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pDisconnect(const std::string &to, const std::string &reason)
{
    std::cout << "[grpc] p2pDisconnect\n";

    P2PDisconnect dmsg;
    dmsg.set_from(m_peer.id());
    dmsg.set_to(to);
    dmsg.set_reason(reason);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pDisconnect(&context, dmsg, &resp);
    if (!status.ok())
    {
        std::cout << "[grpc] " << status.error_code() << " : " << status.error_message() << std::endl;
        return false;
    }

    return true;
}

} // namespace dc
