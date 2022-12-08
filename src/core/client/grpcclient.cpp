//******************************************************************************
//******************************************************************************

#include "core/grpcclient.h"

#include <fmt/core.h>

//******************************************************************************
//******************************************************************************
namespace dc
{

//******************************************************************************
//******************************************************************************
void GRpcClient::subscribe(PeerType type, on_message_handler_t handler)
{
    fmt::print("[grpc] subscribe({})\n", type);

    if (m_reader.joinable())
    {
        fmt::print(stderr, "[grpc] already subscribed\n");
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
            // fmt::print("Message from: {} data: {}\n", m.from(), m.data());

            if (m_messageHandler)
            {
                m_messageHandler(m);
            }
        }

        grpc::Status status = reader->Finish();
        if (!status.ok())
        {
            fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        }
        m_messageHandler = nullptr;
        fmt::print("[grpc] subscribe finished\n");
    });
}

//******************************************************************************
//******************************************************************************
void GRpcClient::updateType(PeerType newType)
{
    fmt::print("[grpc] updatePeer({})\n", newType);

    m_peer.set_type(newType);

    updatePeer();
}

//******************************************************************************
//******************************************************************************
void GRpcClient::updateGroup(std::string newGroup)
{
    fmt::print("[grpc] updateGroup({})\n", newGroup);

    m_peer.set_group(newGroup);

    updatePeer();
}

//******************************************************************************
//******************************************************************************
void GRpcClient::updatePeer()
{
    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->updatePeer(&context, m_peer, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return;
    }
}

//******************************************************************************
//******************************************************************************
void GRpcClient::unsubscribe()
{
    fmt::print("[grpc] {}\n", __func__);

    if (!m_reader.joinable())
    {
        fmt::print("[grpc] already unsubscribed\n");
        return;
    }

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->unsubscribe(&context, m_peer, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
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
    fmt::print("[grpc] {} to: {} data: {}\n", __func__, to, data);

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
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return;
    }
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pInit(const std::string &to, const std::string &sdp)
{
    fmt::print("[grpc] {} to: {} sdp: {}\n", __func__, to, sdp);

    P2PInit initmsg;
    initmsg.set_from(m_peer.id());
    initmsg.set_to(to);
    initmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pInit(&context, initmsg, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pAccept(const std::string &to, const std::string &sdp)
{
    fmt::print("[grpc] {} to: {} sdp: {}\n", __func__, to, sdp);

    P2PAccept accmsg;
    accmsg.set_from(m_peer.id());
    accmsg.set_to(to);
    accmsg.set_sdp(sdp);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pAccept(&context, accmsg, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pCandidate(const std::string &to, const std::string &candidate)
{
    fmt::print("[grpc] {} to: {} candidate: {}\n", __func__, to, candidate);

    P2PCandidate cndmsg;
    cndmsg.set_from(m_peer.id());
    cndmsg.set_to(to);
    cndmsg.set_candidate(candidate);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pCandidate(&context, cndmsg, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return false;
    }

    return true;
}

//******************************************************************************
//******************************************************************************
bool GRpcClient::p2pDisconnect(const std::string &to, const std::string &reason)
{
    fmt::print("[grpc] {} to: {} reason: {}\n", __func__, to, reason);

    P2PDisconnect dmsg;
    dmsg.set_from(m_peer.id());
    dmsg.set_to(to);
    dmsg.set_reason(reason);

    Empty resp;

    grpc::ClientContext context;
    grpc::Status status = m_stub->p2pDisconnect(&context, dmsg, &resp);
    if (!status.ok())
    {
        fmt::print(stderr, "[grpc] {} error: {} {}\n", __func__, status.error_code(), status.error_message());
        return false;
    }

    return true;
}

} // namespace dc
