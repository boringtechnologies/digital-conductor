#include <gst/app/app.h>

#include <iterator>
#include <memory>
#include <thread>

#include "gst/app/gstappsink.h"

#include "core/media/sync.hpp"

#include "internal.hpp"

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("Error: %s\n", error->message);
            g_error_free(error);

            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

static GstElement *get_audio_device(const media::CaptureDevice &device)
{
    GstElement *source;
    MAKE_ELEMENT(source, device.name.c_str(), "publisher-audiosrc");
    return source;
}

static GstElement *get_video_device(const media::CaptureDevice &device)
{
    GstElement *source;

    std::cout << "use " << device.name << std::endl;
    MAKE_ELEMENT(source, device.name.c_str(), "publisher-videosrc");
    if (!device.params.empty())
    {
        std::cout <<     "with params:" << std::endl;
        for (const std::pair<std::string, std::string> & param : device.params)
        {
            std::cout << "        " << param.first << "=" << param.second << std::endl;
            if (param.first == "device-index")
            {
                // TODO temporary ugly hack for mac os users
                // TODO use G_VALUE_TYPE or gst_value_deserialize ...
                g_object_set(G_OBJECT(source), param.first.c_str(), std::stoi(param.second, nullptr, 10), nullptr);
            }
            else
            {
                g_object_set(G_OBJECT(source), param.first.c_str(), param.second.c_str(), nullptr);
            }
        }
    }

    return source;
}

media::Sender::Sender(const std::string &id, media::MediaType type,
                      const media::CaptureDevice &device,
                      const media::ClockInfo &clock)
    : thread(nullptr), write_cbs() {
    GstBus *bus;
    GstElement *source, *conv, *rate, *capsf, *encoder, *payloader, *queue,
        *fakesink;
    gst_init(nullptr, nullptr);
    gst_register_static_plugins();

    loop = g_main_loop_new(nullptr, FALSE);

    pipeline = gst_pipeline_new(id.c_str());

    if (type == media::MediaType::Audio) {
        source = get_audio_device(device);
        MAKE_ELEMENT(conv, "audioconvert", (id + "-publisher-conv").c_str());
        MAKE_ELEMENT(rate, "audioresample", (id + "-publisher-rate").c_str());
        MAKE_ELEMENT(capsf, "capsfilter", (id + "-publisher-capsf").c_str());
        g_object_set(capsf, "caps",
                     gst_caps_from_string("audio/x-raw,rate=48000"), nullptr);
        MAKE_ELEMENT(encoder, "opusenc", "publisher-audioenc");
        payloader = gst_element_factory_make(
            "rtpopuspay", (id + "-publisher-audiopayload").c_str());
        g_object_set(payloader, "pt", 97, NULL);
    } else if (type == media::MediaType::Video) {
        source = get_video_device(device);
        MAKE_ELEMENT(conv, "videoconvert", (id + "-publisher-conv").c_str());
        MAKE_ELEMENT(rate, "videorate", (id + "-publisher-rate").c_str());
        MAKE_ELEMENT(capsf, "capsfilter", (id + "-publisher-capsf").c_str());
        g_object_set(capsf, "caps",
                     gst_caps_from_string("video/x-raw,framerate=30/1"),
                     nullptr);
        MAKE_ELEMENT(encoder, "vp8enc", (id + "-publisher-videoenc").c_str());
        // Generates a keyframe every 5s, with RTCP feedback in place
        g_object_set(encoder, "deadline", 1, "keyframe-max-dist", 150, nullptr);
        payloader = gst_element_factory_make(
            "rtpvp8pay", (id + "-publisher-videopayload").c_str());
        g_object_set(payloader, "pt", 101, NULL);
    }

    MAKE_ELEMENT(tee, "tee", (id + "-publisher-tee").c_str());
    MAKE_ELEMENT(queue, "queue", (id + "-publisher-queue").c_str());
    MAKE_ELEMENT(fakesink, "fakesink", (id + "-publisher-fakesink").c_str());

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_callback, loop);
    gst_object_unref(bus);

    gst_bin_add_many(GST_BIN(pipeline), source, conv, rate, capsf, encoder,
                     payloader, tee, queue, fakesink, nullptr);

    if (!gst_element_link_many(source, conv, rate, capsf, encoder, payloader,
                               tee, queue, fakesink, nullptr)) {
        g_printerr("failed to link the elements\n");
        return;
    }
    g_print("Elements linked successfully\n");
    GstClock *net_clock = gst_net_client_clock_new(
        "net-clock", clock.address.c_str(), clock.port, 0);
    if (net_clock == nullptr) {
        g_print("Failed to create net clock client\n");
        return;
    }

    /* Wait for the clock to stabilise */
    gst_clock_wait_for_sync(net_clock, GST_CLOCK_TIME_NONE);
    gst_pipeline_use_clock(GST_PIPELINE(pipeline), net_clock);
}

media::Sender::~Sender() {
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
}

void media::Sender::start() {
    thread = std::make_unique<std::thread>([this] {
        g_print("Setting the pipeline to playing\n");
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        g_print("Running mainloop\n");
        g_main_loop_run(loop);
    });
}

void media::Sender::stop() {
    g_main_loop_quit(loop);
    g_print("stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    thread->join();
    thread.reset();
}

struct WriteCallbackCtx {
    media::Sender *sender;
    std::string id;
};

static void write_cb_ctx_free(gpointer data) {
    WriteCallbackCtx *ctx = static_cast<WriteCallbackCtx *>(data);
    delete ctx;
}

static GstFlowReturn new_sample(GstAppSink *sink, gpointer userdata) {
    GstSample *sample;
    WriteCallbackCtx *ctx = static_cast<WriteCallbackCtx *>(userdata);
    media::WriteCallback &write_cb = ctx->sender->write_cbs[ctx->id];

    /* Retrieve the buffer */
    sample = gst_app_sink_pull_sample(sink);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a
         * received buffer */
        GstBuffer *buf = gst_sample_get_buffer(sample);
        GstMapInfo info;
        gst_buffer_map(buf, &info, GST_MAP_READ);

        if (write_cb) {
            write_cb((const uint8_t *)info.data, info.size);
        }

        gst_buffer_unmap(buf, &info);

        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

static void rtpbin_pad_padded(GstElement *ele, GstPad *pad, gpointer userdata) {
    gchar *padname = gst_pad_get_name(pad);
    gchar *prefix = g_strdup_printf("send_rtp_src_");
    g_printerr("%s pad added\n", padname);
    if (g_str_has_prefix(padname, prefix)) {
        GstElement *downstream = (GstElement *)userdata;
        GstPad *sinkpad = gst_element_get_request_pad(downstream, "sink_%u");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }
    g_free(prefix);
    g_free(padname);
}

void media::Sender::addReceiver(const std::string &id,
                                media::WriteCallback write_cb) {
    GstElement *appsink, *rtpbin, *funnel, *rtcp_appsrc;
    GstElement *bin = gst_bin_get_by_name(GST_BIN(pipeline), id.c_str());

    if (bin != nullptr) {
        g_printerr("Receiver id %s already exists\n", id.c_str());
        return;
    }

    bin = gst_bin_new(id.c_str());
    g_assert(bin != nullptr);

    MAKE_ELEMENT(rtpbin, "rtpbin", (id + "-publisher-rtpbin").c_str());
    MAKE_ELEMENT(funnel, "funnel", (id + "-publisher-funnel").c_str());
    MAKE_ELEMENT(appsink, "appsink", (id + "-publisher-appsink").c_str());
    MAKE_ELEMENT(rtcp_appsrc, "appsrc",
                 (id + "-publisher-rtcp-appsrc").c_str());

    if (!bin || !funnel || !rtpbin || !appsink) {
        g_printerr("failed to create elements in addReceiver");
        return;
    }

    // We need to make sure the callback lives through the lifetime of the
    // receiver
    write_cbs[id] = write_cb;

    WriteCallbackCtx *ctx = g_new0(WriteCallbackCtx, 1);
    ctx->sender = this;
    ctx->id = id;
    GstAppSinkCallbacks callbacks = {0};
    callbacks.new_sample = new_sample;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, ctx,
                               write_cb_ctx_free);
    g_object_set(appsink, "emit-signals", FALSE, nullptr);
    g_object_set(rtpbin, "ntp-time-source", 3, nullptr);
    g_object_set(rtpbin, "rtcp-sync-send-time", false, nullptr);
    g_signal_connect(rtpbin, "pad-added", G_CALLBACK(rtpbin_pad_padded),
                     funnel);

    guint port = atoi(id.c_str()) + 1;
    g_object_set(rtcp_appsrc, "format", GST_FORMAT_TIME, nullptr);
    g_object_set(rtcp_appsrc, "do-timestamp", true, nullptr);
    g_object_set(rtcp_appsrc, "is-live", true, nullptr);

    gst_bin_add_many(GST_BIN(bin), rtpbin, funnel, appsink, rtcp_appsrc,
                     nullptr);

    GstPad *rtp_send_sinkpad =
        gst_element_get_request_pad(rtpbin, "send_rtp_sink_%u");
    if (!gst_element_link_pads(rtpbin, "send_rtcp_src_%u", funnel, "sink_%u"))
        g_printerr("Failed to link rtcp src and funnel");

    if (!gst_element_link_pads(rtcp_appsrc, "src", rtpbin, "recv_rtcp_sink_%u"))
        g_printerr("Failed to link rtcp_appsrc and rtcp rcv sink");

    if (!gst_element_link_many(funnel, appsink, nullptr))
        g_print("failed to link funnel and appsink");

    gst_bin_add(GST_BIN(pipeline), bin);
    gst_element_sync_state_with_parent(bin);

    GstPad *bin_ghost_pad = gst_ghost_pad_new("sink", rtp_send_sinkpad);

    gst_element_add_pad(bin, bin_ghost_pad);
    if (!gst_element_link(tee, bin)) g_printerr("failed to link tee and bin");

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                              "pipeline");
}

struct RemoveCallbackCtx {
    media::Sender *sender;
    std::string id;
    GstElement *bin;
};

gboolean set_to_null(gpointer data) {
    RemoveCallbackCtx *ctx = (RemoveCallbackCtx *)data;

    GstStateChangeReturn ret = gst_element_set_state(ctx->bin, GST_STATE_NULL);
    g_print("state change return %d\n", ret);

    GstElement *pipeline = (GstElement *)gst_element_get_parent(ctx->bin);
    gst_bin_remove(GST_BIN(pipeline), ctx->bin);

    gst_object_unref(ctx->bin);
    ctx->sender->write_cbs.erase(ctx->id);
    delete (ctx);

    return FALSE;
}

GstPadProbeReturn probe_call_back(GstPad *pad, GstPadProbeInfo *info,
                                  gpointer user_data) {
    RemoveCallbackCtx *ctx = (RemoveCallbackCtx *)user_data;
    GstPad *peer_pad = gst_pad_get_peer(pad);

    if (!gst_pad_unlink(pad, peer_pad)) {
        g_printerr("failed to unlink pads in %s\n", __func__);
    }
    gst_object_unref(peer_pad);

    // FIXME: this will NOT be scheduled on our internal mainloop
    g_idle_add(set_to_null, ctx);

    return GST_PAD_PROBE_REMOVE;
}

void media::Sender::removeReceiver(const std::string &id) {
    GstElement *bin = gst_bin_get_by_name(GST_BIN(pipeline), id.c_str());
    if (bin == nullptr) {
        g_printerr("No such receiver in the current pipeline %s\n", id.c_str());
        return;
    }

    RemoveCallbackCtx *ctx = new RemoveCallbackCtx();
    ctx->sender = this;
    ctx->id = id;
    ctx->bin = bin;

    GstPad *bin_ghost_pad = gst_element_get_static_pad(bin, "sink");

    GstPad *tee_src_pad = gst_pad_get_peer(bin_ghost_pad);

    gst_pad_add_probe(tee_src_pad, GST_PAD_PROBE_TYPE_IDLE, probe_call_back,
                      ctx, nullptr);

    gst_object_unref(bin_ghost_pad);
    gst_object_unref(tee_src_pad);
}

bool media::Sender::push(const std::string &id, const uint8_t *payload,
                         size_t len) {
    GstState cur_state = GST_STATE_NULL, pending_state;
    GstElement *rtcp_appsrc;
    GstBuffer *buf;
    GstMapInfo info;
    GstFlowReturn ret;

    gst_element_get_state(pipeline, &cur_state, &pending_state, 0);
    if (cur_state != GST_STATE_PAUSED && cur_state != GST_STATE_PLAYING)
        return true;

    rtcp_appsrc = gst_bin_get_by_name(GST_BIN(pipeline),
                                      (id + "-publisher-rtcp-appsrc").c_str());

    buf = gst_buffer_new_allocate(nullptr, len, nullptr);
    if (buf == nullptr) {
        g_printerr("Failed to allocate buffer\n");
        return false;
    }

    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        g_printerr("Failed to get buffer map\n");
        return false;
    }
    memcpy(info.data, payload, len);
    gst_buffer_unmap(buf, &info);

    g_signal_emit_by_name(rtcp_appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        g_printerr("Failed to do 'push-buffer' \n");
        return false;
    }

    return true;
}
