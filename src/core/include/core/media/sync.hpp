#ifndef SYNC_H_
#define SYNC_H_

#include <gst/gst.h>
#include <gst/net/net.h>

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>

// Synced Groups API
//
// The key considerations for the moment are:
//
//  * Receivers will receiver from a single Sender (but clients can
//    instantiate as many Receivers as they want)
//
//  * Each audio and video stream will constitute an indivial Sender (this can
//    make things a little less efficient in number of ports consumed, but we
//    can worry about that when we start doing ICE)
//
//  * We will start an internal event loop thread for each library instance,
//    from which error and other callbacks will be emitted. This does not
//    include other threads created by GStreamer itself.
//

namespace media {
enum MediaType {
    Audio,
    Video,
};

// typedef for the callback fn set by the application, will be called in the
// appsink's context after pull-sample
typedef std::function<void(const uint8_t *, size_t len)> WriteCallback;

// Common clock information for all receivers
struct ClockInfo {
    // XXX: What clock type should we use? GstNetClock or NTP? Do we want to
    // support more than one?
    std::string address;
    uint16_t port;
};

struct SenderInfo {
    // XXX: To be fleshed out, but this will contain any information we want to
    // publish to receivers (such as format information) so they can initialise
    // themselves for the data they will need to decode/render.
};

struct CaptureDevice {
    // XXX: This can be extended for other source types
    std::string name;
};

class Sender {
   private:
    std::unique_ptr<std::thread> thread;
    GMainLoop *loop;
    guint bus_watch_id;
    GstElement *pipeline, *tee;

   public:
    // The constructor takes:
    //   * An instance id, for easier debugging
    //   * A media type (audio/video)
    //   * A device that we can use as the source
    //   * A clock to sync to
    //
    // When constructed, the required resources are allocated, but nothing
    // happens yet. The ::start() method must be called.
    //
    // XXX: Do we want audio and video senders to be completely independent? If
    // not, we can add an audio and video device here, and the type might be
    // redundant
    Sender(const std::string &id, MediaType type, const CaptureDevice &device,
           const ClockInfo &clock);

    // Just tear everything down
    ~Sender();

    // Start streaming media to any connected receivers.
    //
    // XXX: How should we handle errors? We could return an error here, but an
    // error callback might be better.
    void start();

    // Shut down capturing media and streaming to any connected receivers.
    void stop();

    // Have the sender stream to the receiver at the specified address/port
    // pair. The id is handy as a handle to remove the receiver later.
    //
    // XXX: We might choose to mux RTCP from the get-go so we don't have to
    // reserve more than one port at a time, which should make ICE easier.
    void addReceiver(const std::string &id, WriteCallback cb);

    void removeReceiver(const std::string &id);

    // FIXME: Can we make this private?
    std::map<std::string, WriteCallback> write_cbs;
    // Push the data to the appsrc for the given receiver id
    // returns TRUE is push is successful
    // returns FALSE in case of an error
    bool push(const std::string &id, const uint8_t *payload, size_t len);
};

struct PlaybackDevice {
    // XXX: We can render to an audio device or a separate video window easily
    // enough, but we need a good way to provide a platform-specific surface to
    // render within the application itself.
};

class Receiver {
   private:
    std::unique_ptr<std::thread> thread;
    GMainLoop *loop;
    guint bus_watch_id;
    GstElement *pipeline, *appsrc;
    // XXX: Make the instantation of the Clock global to all the receivers in a
    // single app

   public:
    // The constructor takes:
    //   * An instance id, for easier debugging
    //   * A media type (audio/video)
    //   * A device to play to
    //   * A clock to sync to
    //   * The total delay to playback with for sync
    //
    // When constructed, the required resources are allocated, but nothing
    // happens yet. The ::start() method must be called.
    //
    // XXX: If a Sender sends audio and video, this API will need to change too.
    Receiver(const std::string &id, MediaType type,
             const PlaybackDevice &device, const ClockInfo &clock,
             std::chrono::duration<int, std::milli> delay, bool synchronize,
             WriteCallback write_cb);

    // Just tear everything down
    ~Receiver();

    // Start streaming media from any connected senders.
    //
    // XXX: How should we handle errors? We could return an error here, but an
    // error callback might be better.
    void start();

    // Shut down reading media from senders.
    void stop();

    // Push the data to the appsrc
    // returns TRUE is push is successful
    // returns FALSE in case of an error
    bool push(const uint8_t *payload, size_t len);

    WriteCallback write_cb;
};
}  // namespace media

#endif  // SYNC_H_
