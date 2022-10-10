#include <gst/app/app.h>

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

static GstFlowReturn new_sample(GstAppSink *sink, gpointer userdata) {
    GstSample *sample;
    media::Receiver *self = static_cast<media::Receiver *>(userdata);

    /* Retrieve the buffer */
    sample = gst_app_sink_pull_sample(sink);
    if (sample) {
        /* The only thing we do in this example is print a * to indicate a
         * received buffer */
        GstBuffer *buf = gst_sample_get_buffer(sample);
        GstMapInfo info;
        gst_buffer_map(buf, &info, GST_MAP_READ);

        if (self->write_cb) {
            self->write_cb((const uint8_t *)info.data, info.size);
        }

        gst_buffer_unmap(buf, &info);

        gst_sample_unref(sample);
        return GST_FLOW_OK;
    }

    return GST_FLOW_ERROR;
}

static void rtpbin_pad_padded(GstElement *ele, GstPad *pad, gpointer userdata) {
    gchar *padname = gst_pad_get_name(pad);
    gchar *prefix = g_strdup_printf("recv_rtp_src_");
    g_printerr("%s pad added\n", padname);
    if (g_str_has_prefix(padname, prefix)) {
        GstElement *downstream = (GstElement *)userdata;
        GstPad *sinkpad = gst_element_get_static_pad(downstream, "sink");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }
    g_free(prefix);
    g_free(padname);
}

static GstElement *get_audio_device(const std::string &id) {
    GstElement *sink;

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
                          std::chrono::duration<int, std::milli> delay,
                          bool synchronize, media::WriteCallback write_cb)
    : thread(nullptr), write_cb(write_cb)
{
    GstBus *bus;
    GstElement *rtpbin, *depayloader, *decoder, *sink, *appsink;
    gst_init(nullptr, nullptr);
    gst_register_static_plugins();

    loop = g_main_loop_new(nullptr, FALSE);

    pipeline = gst_pipeline_new(("rx-" + id).c_str());
    MAKE_ELEMENT(appsrc, "appsrc", ("rx-" + id + "-appsrc").c_str());
    g_object_set(appsrc, "format", GST_FORMAT_TIME, nullptr);
    g_object_set(appsrc, "do-timestamp", true, nullptr);
    g_object_set(appsrc, "is-live", true, nullptr);
    MAKE_ELEMENT(rtpbin, "rtpbin", ("rx-" + id + "-rtpbin").c_str());
    if (type == media::MediaType::Audio) {
        MAKE_ELEMENT(depayloader, "rtpopusdepay",
                     ("rx-" + id + "-depay").c_str());
        MAKE_ELEMENT(decoder, "opusdec", ("rx-" + id + "-dec").c_str());
        sink = get_audio_device(id);
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
        MAKE_ELEMENT(sink, "autovideosink", ("rx-" + id + "-sink").c_str());
        g_object_set(
            appsrc, "caps",
            gst_caps_from_string("application/"
                                 "x-rtp,media=(string)video,clock-rate=(int)"
                                 "90000,encoding-name=(string)VP8,payload=101"),
            nullptr);
    }

    MAKE_ELEMENT(appsink, "appsink", ("rx-" + id + "-appsink-rtcp").c_str());
    GstAppSinkCallbacks callbacks = {0};
    callbacks.new_sample = new_sample;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, this, NULL);
    g_object_set(appsink, "emit-signals", FALSE, nullptr);

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
        gst_element_get_request_pad(rtpbin, "recv_rtp_sink_%u");

    GstPad *recv_rtcp_sink_pad =
        gst_element_get_request_pad(rtpbin, "recv_rtcp_sink_%u");

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_callback, loop);
    gst_object_unref(bus);

    gst_bin_add_many(GST_BIN(pipeline), appsrc, rtpbin, depayloader, decoder,
                     sink, appsink, nullptr);

    gst_pad_link(src_pad, recv_rtp_sink_pad);
    gst_element_link_pads(rtpbin, "send_rtcp_src_%u", appsink, "sink");
    if (!gst_element_link_many(depayloader, decoder, sink, nullptr)) {
        g_printerr("Failed to link the elements\n");
        return;
    }

    if (synchronize) {
        GstClock *net_clock = gst_net_client_clock_new(
            nullptr, clock.address.c_str(), clock.port, 0);
        if (net_clock == NULL) {
            g_print("Failed to create net clock client\n");
            return;
        }

        /* Wait for the clock to stabilise */
        gst_clock_wait_for_sync(net_clock, GST_CLOCK_TIME_NONE);
        gst_pipeline_use_clock(GST_PIPELINE(pipeline), net_clock);
        /* Set this high enough so that it's higher than the minimum latency
         * on all receivers */
        gst_pipeline_set_latency(GST_PIPELINE(pipeline),
                                 delay.count() * GST_MSECOND);
    }
}

media::Receiver::~Receiver() {
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);
}

void media::Receiver::start() {
    thread = std::make_unique<std::thread>([this] {
        g_print("Setting the pipeline to playing\n");
        gst_element_set_state(pipeline, GST_STATE_PLAYING);

        g_print("Running mainloop\n");

        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                                  "rx-pipeline");
        g_main_loop_run(loop);
        GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                                  "error");
    });
}

void media::Receiver::stop() {
    g_main_loop_quit(loop);

    g_print("stopping playback\n");
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
        g_printerr("Failed to allocate buffer\n");
        return false;
    }

    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        g_printerr("Failed to get buffer map\n");
        return false;
    }
    memcpy(info.data, payload, len);
    gst_buffer_unmap(buf, &info);

    g_signal_emit_by_name(appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        g_printerr("Failed to do 'push-buffer' \n");
        return false;
    }

    return true;
}
