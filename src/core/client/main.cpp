//******************************************************************************
//******************************************************************************

#include "core/grpcclient.h"
#include "core/juiceclient.h"
#include "core/media/sync.hpp"

#include <iostream>

//******************************************************************************
//******************************************************************************
std::shared_ptr<dc::GRpcClient> grclient;

typedef std::shared_ptr<dc::JuiceClient> jcclient_ptr_t;
std::map<std::string, jcclient_ptr_t> jcclients;

// sender
media::ClockInfo mediaclock = {"66.228.38.186", 10000};
media::MediaType mediatype  = media::MediaType::Video;
media::CaptureDevice captureDevice;
std::shared_ptr<media::Sender> mediasender;

// receiver
media::PlaybackDevice playbackDevice;
media::ClockInfo receiverclock = {"66.228.38.186", 10000};
std::chrono::duration<int> delay;
std::shared_ptr<media::Receiver> mediareceiver;

//******************************************************************************
//******************************************************************************
void onStateChanged(juice_agent_t *agent, juice_state_t state, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);
    std::string id          = caller->id();

    // std::cout << "onStateChanged " << state << " " << juice_state_to_string(state) << std::endl;
    std::cout << "[juice-main] onStateChanged "
              << "P2P connection <" << state << "> " << juice_state_to_string(state) << std::endl;

    if (state == JUICE_STATE_COMPLETED)
    {
        std::cout << "[juice-main] onStateChanged "
                  << "P2P connection, " << id << " connected" << std::endl;

        //        if (!mediasender)
        //        {
        //            std::cout << "    no media" << std::endl;
        //        }
        //        else
        //        {
        //            if (jcclients.find(id) == jcclients.end() || !jcclients[id])
        //            {
        //                std::cout << "    no receiver named " << id << std::endl;
        //            }
        //            else
        //            {
        //                mediasender->addReceiver(id, [id](const uint8_t *data, size_t len)
        //                {
        //                    jcclients[id]->sendMessage(data, len);
        //                    // std::cout << len << " bytes to " << id << std::endl;
        //                });
        //            }
        //        }
    }
    else if (state == JUICE_STATE_DISCONNECTED)
    {
        if (mediasender)
        {
            mediasender->removeReceiver(id);
        }
        if (mediareceiver)
        {
            mediareceiver->stop();
            mediareceiver.reset();
        }
        std::cout << "[juice-main] onStateChanged "
                  << "P2P connection, " << id << " disconnected" << std::endl;
    }
}

//******************************************************************************
//******************************************************************************
void onCandidate(juice_agent_t *agent, const char *sdp, void *user_ptr)
{
    // std::cout << "onCandidate " << sdp << std::endl;

    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    if (grclient)
    {
        grclient->sendMessage(caller->id(), std::string("JP2PC ") + sdp);
    }
}

//******************************************************************************
//******************************************************************************
void onGatheringDone(juice_agent_t *agent, void *user_ptr)
{
    //    std::cout << "onGatheringDone" << std::endl;
}

//******************************************************************************
//******************************************************************************
void onRecv(juice_agent_t *agent, const char *data, size_t size, void *user_ptr)
{
    dc::JuiceClient *caller = reinterpret_cast<dc::JuiceClient *>(user_ptr);

    // std::cout << size << " bytes from " << caller->id() << std::endl;
    // std::cout << "    " << std::string(data, size) << std::endl;

    if (mediareceiver)
    {
        mediareceiver->push(reinterpret_cast<const uint8_t *>(data), size);
    }
    else if (mediasender)
    {
        for (const auto &jc : jcclients)
        {
            if (jc.second.get() == caller)
            {
                mediasender->push(jc.first, reinterpret_cast<const uint8_t *>(data), size);
            }
        }
    }
}

//******************************************************************************
//******************************************************************************
jcclient_ptr_t getJccClient(const std::string &id)
{
    if (jcclients.count(id) == 0)
    {
        // std::cout << "message to unknown " << to << std::endl;
        return jcclient_ptr_t();
    }

    return jcclients[id];
}

//******************************************************************************
//******************************************************************************
void onGrpcMessage(const std::string &from, const std::string &to, const std::string &msg)
{
    // std::cout << "Message from " << from << " --> " << msg << std::endl;

    // TODO move juice messages to another instance
    if (msg.rfind("JP2PI", 0) == 0)
    {
        // juice init connection, sdp
        jcclient_ptr_t ptr = getJccClient(from);
        if (ptr)
        {
            std::cout << "[main] onGrpcMessage "
                      << "juice already initialized " << from << std::endl;
            return;
        }

        ptr.reset(new dc::JuiceClient(from, &onStateChanged, &onCandidate, &onGatheringDone, &onRecv, &onRecv));
        ptr->init(msg.substr(6));

        if (!grclient->p2pAccept(from, ptr->sdp()))
        {
            std::cout << "[main] onGrpcMessage "
                      << "p2p not accepted " << from << std::endl;
            return;
        }

        jcclients[from] = ptr;
    }
    else if (msg.rfind("JP2PA", 0) == 0)
    {
        // juice connection accepted, sdp
        jcclient_ptr_t ptr = getJccClient(from);
        if (!ptr)
        {
            std::cout << "[main] onGrpcMessage "
                      << "juice not initialized " << from << std::endl;
            return;
        }

        ptr->connect(msg.substr(6));
    }
    else if (msg.rfind("JP2PC", 0) == 0)
    {
        // juice candidate
        jcclient_ptr_t ptr = getJccClient(from);
        if (!ptr)
        {
            std::cout << "[main] onGrpcMessage "
                      << "juice not initialized " << from << std::endl;
            return;
        }
        ptr->addRemote(msg.substr(6));
    }
    else if (msg.rfind("JP2PD", 0) == 0)
    {
        // juice disconnect
        if (mediasender)
        {
            mediasender->removeReceiver(from);
        }
        std::cout << "[main] onGrpcMessage "
                  << "juice disconnect from " << from << " reason: " << msg.substr(6) << std::endl;
        jcclients.erase(from);
    }
    else
    {
        // simple message
        std::cout << "[main] onGrpcMessage "
                  << "msg from <" << from << "> " << msg << std::endl;
    }
}

//******************************************************************************
//******************************************************************************
void printHelp()
{
    std::cout << "****************************************" << std::endl
              << "commands list:" << std::endl
              << "  exit" << std::endl
              << "  help" << std::endl
              << "  getPeerInfo" << std::endl
              << "  subscribe user_id|group_id|type (possible r, w, rw)"
              << std::endl
              //              << "  sendMessage user_id|message" << std::endl
              << "  unsubscribe" << std::endl
              << "  juiceConnect user_id" << std::endl
              << "  juiceState user_id" << std::endl
              << "  juiceSend user_id|message" << std::endl
              << "  juiceDisconnect user_id" << std::endl
              << "  startstream device (/dev/video0 by default)" << std::endl
              << "  transmitto user_id" << std::endl
              << "  receivestream user_id" << std::endl
              << "  stopstream" << std::endl
              << "****************************************" << std::endl;
}

//******************************************************************************
//******************************************************************************
std::vector<std::string> parseCommand(const std::string &cmd)
{
    std::istringstream ss(cmd);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, '|'))
    {
        elems.push_back(item);
    }
    return elems;
}

//******************************************************************************
//******************************************************************************
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "try " << argv[0] << " ip:port <localhost:50051>" << std::endl;
        return 0;
    }

    std::string target = argv[1];

    printHelp();

    while (true)
    {
        std::string command;
        std::cout << "> ";
        std::cin >> command;
        if (command == "exit")
        {
            break;
        }
        else if (command == "help")
        {
            printHelp();
            continue;
        }
        else if (command == "getPeerInfo")
        {
            if (grclient)
            {
                grclient->getPeerList();
            }
            else
            {
                grclient.reset(new dc::GRpcClient("none", "none",
                                                  grpc::CreateChannel(target, grpc::InsecureChannelCredentials())));
                grclient->getPeerList();
                grclient.reset();
            }
            continue;
        }
        else if (command == "subscribe")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            std::string group = "group";
            if (v.size() >= 2)
            {
                group = v[1];
            }

            uint32_t type = 3;
            if (v.size() > 2 && v[2].size() > 0)
            {
                type = v[2] == "r" ? 1 : v[2] == "w" ? 2 : v[2] == "rw" ? 3 : 0;
            }

            if (type == 0 || type > 3)
            {
                std::cout << "wrong type, try r, w or rw" << std::endl;
                continue;
            }

            grclient.reset(
                new dc::GRpcClient(v[0], group, grpc::CreateChannel(target, grpc::InsecureChannelCredentials())));
            grclient->subscribe((dc::PeerType)type, onGrpcMessage);
        }
        else if (command == "unsubscribe")
        {
            if (grclient)
            {
                // disconnect all p2p connections
                for (const std::pair<std::string, jcclient_ptr_t> &item : jcclients)
                {
                    if (item.second)
                    {
                        if (!grclient->p2pDisconnect(item.first, "unsubscribe"))
                        {
                            std::cout << "p2p disconnect error - " << item.first << std::endl;
                        }

                        item.second->disconnect();
                    }
                    jcclients.erase(item.first);
                }
                // grclient->unsubscribe();
                grclient.reset();
            }
        }
        else if (command == "juiceConnect")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            if (!grclient)
            {
                std::cout << "not subscribed" << std::endl;
                continue;
            }

            jcclient_ptr_t ptr(
                new dc::JuiceClient(v[0], &onStateChanged, &onCandidate, &onGatheringDone, &onRecv, &onRecv));
            ptr->init();

            if (!grclient->p2pInit(v[0], ptr->sdp()))
            {
                std::cout << "p2p init error" << std::endl;
                continue;
            }

            jcclients[v[0]] = ptr;
        }
        else if (command == "juiceState")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            if (!jcclients.count(v[0]))
            {
                std::cout << "not found" << std::endl;
                continue;
            }

            juice_state_t s = jcclients[v[0]]->state();
            std::cout << s << " " << juice_state_to_string(s) << std::endl;
        }
        else if (command == "juiceSend")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 2)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            if (!jcclients.count(v[0]))
            {
                std::cout << "not connected" << std::endl;
                continue;
            }

            jcclients[v[0]]->sendMessage(v[1]);
        }
        else if (command == "juiceDisconnect")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            if (!jcclients.count(v[0]))
            {
                std::cout << "not connected" << std::endl;
                continue;
            }

            if (!grclient->p2pDisconnect(v[0], "user request"))
            {
                std::cout << "p2p disconnect error" << std::endl;
                continue;
            }

            jcclients[v[0]]->disconnect();
            jcclients.erase(v[0]);
        }
        else if (false && command == "sendMessage")
        {
            if (!grclient)
            {
                std::cout << "not subscribed" << std::endl;
                continue;
            }

            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() != 2)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            grclient->sendMessage(v[0], v[1]);
        }
        else if (command == "startstream")
        {
            captureDevice.name = "/dev/video0";

            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << captureDevice.name << " used by default" << std::endl;
            }
            else
            {
                captureDevice.name = v[0];
            }

            if (!grclient)
            {
                std::cout << "not subscribed" << std::endl;
                continue;
            }
            if (mediasender)
            {
                std::cout << "already started" << std::endl;
                continue;
            }

            mediasender.reset(new media::Sender(grclient->id(), mediatype, captureDevice, mediaclock));
            mediasender->start();
        }
        else if (command == "transmitto")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }
            if (!mediasender)
            {
                std::cout << "not started" << std::endl;
                continue;
            }
            const std::string id = v[0];
            if (jcclients.find(id) == jcclients.end() || !jcclients[id])
            {
                std::cout << "no receiver named " << id << std::endl;
            }
            else
            {
                mediasender->addReceiver(id, [id](const uint8_t *data, size_t len) {
                    jcclients[id]->sendMessage(data, len);
                    // std::cout << len << " bytes to " << id << std::endl;
                });
            }
            std::cout << "started to " << id << std::endl;
        }
        else if (command == "receivestream")
        {
            std::string params;
            std::cin >> params;
            std::vector<std::string> v = parseCommand(params);
            if (v.size() < 1)
            {
                std::cout << "wrong params, try help" << std::endl;
                continue;
            }

            if (mediareceiver)
            {
                std::cout << "already receiving" << std::endl;
                continue;
            }

            std::string id   = v[0];
            bool synchronize = true;
            mediareceiver.reset(new media::Receiver(id, mediatype, playbackDevice, receiverclock, delay, synchronize,
                                                    [id](const uint8_t *data, size_t len) {
                                                        if (mediatype == media::MediaType::Audio)
                                                        {
                                                            jcclients[id]->sendAudio(data, len);
                                                        }
                                                        else if (mediatype == media::MediaType::Video)
                                                        {
                                                            jcclients[id]->sendVideo(data, len);
                                                        }
                                                        // std::cout << len << " bytes to " << id << std::endl;
                                                    }));
            mediareceiver->start();
        }
        else if (command == "stopstream")
        {
            if (mediasender)
            {
                mediasender->stop();
                mediasender.reset();
            }
            if (mediareceiver)
            {
                mediareceiver->stop();
                mediareceiver.reset();
            }

            std::cout << "all stopped" << std::endl;
            continue;
        }
        else
        {
            std::cout << "wrong, try help" << std::endl;
        }
    }

    if (mediasender)
    {
        mediasender->stop();
        mediasender.reset();
    }

    for (const std::pair<std::string, jcclient_ptr_t> &it : jcclients)
    {
        if (!grclient->p2pDisconnect(it.first, "destroyed"))
        {
            std::cout << "p2p disconnect error" << std::endl;
            continue;
        }

        it.second->disconnect();
    }

    return 0;
}
