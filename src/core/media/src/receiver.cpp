#include <gst/app/app.h>

#include "core/media/sync.hpp"

#include "internal.hpp"

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("[gst-receiver] end of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("[gst-receiver] error: %s\n", error->message);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }

    return TRUE;
}

static void rtpbin_pad_padded(GstElement *ele, GstPad *pad, gpointer userdata) {
    gchar *padname = gst_pad_get_name(pad);
    gchar *prefix = g_strdup_printf("recv_rtp_src_");
    g_printerr("[gst-receiver] %s pad added in %s\n", padname, __func__);
    if (g_str_has_prefix(padname, prefix)) {
        GstElement *downstream = (GstElement *)userdata;
        GstPad *sinkpad = gst_element_get_static_pad(downstream, "sink");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }
    g_free(prefix);
    g_free(padname);
}

static void actual_sink_added_callback(GstChildProxy *, GstBaseSink * actual_sink, gchar *, gpointer) {
    std::cout << "[gst-receiver] " << __func__ << '\n';

    g_object_set(actual_sink, "force-aspect-ratio", TRUE, NULL);
}

static GstElement *get_audio_device(const media::PlaybackDevice &device, const std::string &id) {
    GstElement *sink;

    // TODO use device for element creation

#if defined(__linux__)
    MAKE_ELEMENT(sink, "pulsesink", ("rx-" + id + "-sink").c_str());
#elif defined(__APPLE__)
    MAKE_ELEMENT(sink, "osxaudiosink", ("rx-" + id + "-sink").c_str());
#endif

    g_object_set(sink, "drift-tolerance", 10000, NULL);
    g_object_set(sink, "alignment-threshold", 10000, NULL);

    return sink;
}

media::Receiver::Receiver(const std::string &id, MediaType type,
                          const PlaybackDevice &device, const ClockInfo &clock,
                          std::chrono::duration<int, std::milli> delay, bool synchronize)
    : media_type(type), thread(nullptr)
{
    GstBus *bus;
    GstElement *rtpbin, *depayloader, *decoder, *convert, *sink;

    loop = g_main_loop_new(nullptr, FALSE);

    pipeline = gst_pipeline_new(("rx-" + id + (media_type ? "-video" : "-audio")).c_str());
    MAKE_ELEMENT(appsrc, "appsrc", ("rx-" + id + "-appsrc").c_str());
    g_object_set(appsrc, "format", GST_FORMAT_TIME, nullptr);
    g_object_set(appsrc, "do-timestamp", true, nullptr);
    g_object_set(appsrc, "is-live", true, nullptr);
    MAKE_ELEMENT(rtpbin, "rtpbin", ("rx-" + id + "-rtpbin").c_str());
    if (type == media::MediaType::Audio) {
        MAKE_ELEMENT(depayloader, "rtpopusdepay", ("rx-" + id + "-depay").c_str());
        MAKE_ELEMENT(decoder, "opusdec", ("rx-" + id + "-dec").c_str());
        MAKE_ELEMENT(convert, "audioconvert", ("rx-" + id + "-cvrt").c_str());
        sink = get_audio_device(device, id);
        g_object_set(
            appsrc, "caps",
            gst_caps_from_string("application/"
                                 "x-rtp,media=(string)audio,clock-rate=(int)"
                                 "48000,encoding-name=(string)OPUS,payload=97"),
            nullptr);

    } else if (type == media::MediaType::Video) {
        MAKE_ELEMENT(depayloader, "rtpvp8depay",
                     ("rx-" + id + "-depay").c_str());
        MAKE_ELEMENT(decoder, "vp8dec", ("rx-" + id + "-dec").c_str());
        MAKE_ELEMENT(convert, "videoconvert", ("rx-" + id + "-cvrt").c_str());
        MAKE_ELEMENT(sink, "autovideosink", ("rx-" + id + "-sink").c_str());
        g_signal_connect(sink, "child-added", G_CALLBACK(actual_sink_added_callback), NULL);
        g_object_set(
            appsrc, "caps",
            gst_caps_from_string("application/"
                                 "x-rtp,media=(string)video,clock-rate=(int)"
                                 "90000,encoding-name=(string)VP8,payload=101"),
            nullptr);
    }

    g_signal_connect(rtpbin, "pad-added", G_CALLBACK(rtpbin_pad_padded),
                     depayloader);
    if (synchronize) {
        g_object_set(rtpbin, "buffer-mode", 4, "ntp-time-source", 3, "ntp-sync",
                     true, nullptr);
    } else {
        g_object_set(rtpbin, "buffer-mode", 0, nullptr);
    }

    GstPad *src_pad = gst_element_get_static_pad(appsrc, "src");
    GstPad *recv_rtp_sink_pad =
        gst_element_request_pad_simple(rtpbin, "recv_rtp_sink_%u");

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_callback, loop);
    gst_object_unref(bus);

    gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpbin, depayloader, decoder,
                     convert, sink, nullptr);

    gst_pad_link(src_pad, recv_rtp_sink_pad);
    if (!gst_element_link_many(depayloader, decoder, convert, sink, nullptr)) {
        g_printerr("[gst-receiver-%d] failed to link the elements\n", media_type);
        return;
    }

    if (synchronize) {
        GstClock *net_clock = gst_net_client_clock_new(
            nullptr, clock.address.c_str(), clock.port, 0);
        if (net_clock == NULL) {
            g_print("[gst-receiver-%d] failed to create net clock client\n", media_type);
            return;
        }

        /* Wait for the clock to stabilise */
        gst_clock_wait_for_sync(net_clock, GST_CLOCK_TIME_NONE);
        gst_pipeline_use_clock(GST_PIPELINE(pipeline), net_clock);
        /* Set this high enough so that it's higher than the minimum latency
         * on all receivers */

        long latency = delay.count() * GST_MSECOND;

        g_print("[gst-receiver-%d] reciever latency %ld ns\n", media_type, latency);

        gst_pipeline_set_latency(GST_PIPELINE(pipeline), latency);
    }
}

media::Receiver::~Receiver() {
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
    g_print("[gst-reciever-%d] ~deconstructed\n", media_type);
}

void media::Receiver::start() {
    thread = std::make_unique<std::thread>([this] {
        g_print("[gst-receiver-%d] Setting the pipeline to playing\n", media_type);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        g_print("[gst-receiver-%d] Running mainloop\n", media_type);

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                                  (std::string("rx-pipeline-") + (media_type ? "video" : "audio")).c_str());
        g_main_loop_run(loop);
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                                  (std::string("stopped-") + (media_type ? "video" : "audio")).c_str());
    });
}

void media::Receiver::stop() {
    g_main_loop_quit(loop);

    g_print("[gst-receiver-%d] stopping playback\n", media_type);
    gst_element_set_state(pipeline, GST_STATE_NULL);

    thread->join();
    thread.reset();
}

bool media::Receiver::push(const uint8_t *payload, size_t len) {
    GstState cur_state = GST_STATE_NULL, pending_state;
    GstBuffer *buf;
    GstMapInfo info;
    GstFlowReturn ret;

    gst_element_get_state(pipeline, &cur_state, &pending_state, 0);
    if (cur_state != GST_STATE_PAUSED && cur_state != GST_STATE_PLAYING)
        return true;

    buf = gst_buffer_new_allocate(nullptr, len, nullptr);
    if (buf == nullptr) {
        g_printerr("[gst-receiver-%d] Failed to allocate buffer\n", media_type);
        return false;
    }

    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        g_printerr("[gst-receiver-%d] Failed to get buffer map\n", media_type);
        return false;
    }
    memcpy(info.data, payload, len);
    gst_buffer_unmap(buf, &info);

    g_signal_emit_by_name(appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        g_printerr("[gst-receiver-%d] Failed to do 'push-buffer'\n", media_type);
        return false;
    }

    return true;
}
