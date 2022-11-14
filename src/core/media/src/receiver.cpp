#include <gst/app/app.h>
#include <gst/net/net.h>

#include "core/media/sync.hpp"
#include "internal.hpp"

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
    fmt::print("[gst-receiver] {} pad added in {}\n", padname, __func__);
    if (g_str_has_prefix(padname, prefix)) {
        GstElement *downstream = (GstElement *)userdata;
        GstPad *sinkpad = gst_element_get_static_pad(downstream, "sink");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }
    g_free(prefix);
    g_free(padname);
}

static void rtpbin_new_jitterbuffer_callback(GstElement *,
                                             GstElement *jitterbuffer, guint,
                                             guint, gpointer *user_data) {
    const char *name = (const char *)user_data;
    fmt::print("[gst-receiver-{}] {}\n", name, __func__);
    g_object_set(jitterbuffer, "latency", 0, "drop-on-latency", FALSE, NULL);
}

static void actual_sink_added_callback(GstChildProxy *,
                                       GstBaseSink *actual_sink, gchar *,
                                       gpointer) {
    fmt::print("[gst-receiver] {}\n", __func__);

    g_object_set(actual_sink, "force-aspect-ratio", TRUE, NULL);
}

struct media::Receiver::impl final : BasePipeline {
    friend class media::Receiver;

    GstElement *appsrc;
    const char *media_name;

    impl(const std::string &name, media::MediaType type)
        : BasePipeline(name + "-rcv-" + (type ? "video" : "audio")),
          appsrc(NULL) {}

    ~impl() override {}
};

media::Receiver::Receiver(const std::string &id, MediaType type,
                          const PlaybackDevice &device, const ClockInfo &clock,
                          std::chrono::duration<int, std::milli> delay,
                          bool synchronize, media::WriteCallback write_cb)
    : m_p(new impl(id.c_str(), type)), media_type(type), write_cb(write_cb) {
    GstElement *rtpbin, *depayloader, *decoder, *convert, *sink, *appsink;

    const std::string &name_prefix = m_p->name;

    m_p->appsrc = m_p->makeElement("appsrc");
    g_object_set(m_p->appsrc, "format", GST_FORMAT_TIME, "do-timestamp", true,
                 "is-live", true, nullptr);
    rtpbin = m_p->makeElement("rtpbin");
    g_signal_connect(rtpbin, "new-jitterbuffer",
                     G_CALLBACK(rtpbin_new_jitterbuffer_callback),
                     (gpointer)m_p->name.c_str());

    if (type == media::MediaType::Audio) {
        depayloader = m_p->makeElement("rtpvorbisdepay");
        decoder = m_p->makeElement("vorbisdec");
        convert = m_p->makeElement("audioconvert");
        g_object_set(m_p->appsrc, "caps",
                     gst_caps_from_string(
                         "application/"
                         "x-rtp,media=(string)audio,clock-rate=(int)"
                         "48000,encoding-name=(string)VORBIS,payload=97"),
                     nullptr);

        convert = m_p->makeElement("audioconvert");
        sink = m_p->makeElement("autoaudiosink");
    } else if (type == media::MediaType::Video) {
        depayloader = m_p->makeElement("rtpvp8depay");
        decoder = m_p->makeElement("vp8dec");
        convert = m_p->makeElement("videoconvert");
        sink = m_p->makeElement("autovideosink");
        g_signal_connect(sink, "child-added",
                         G_CALLBACK(actual_sink_added_callback), NULL);
        g_object_set(
            m_p->appsrc, "caps",
            gst_caps_from_string("application/"
                                 "x-rtp,media=(string)video,clock-rate=(int)"
                                 "90000,encoding-name=(string)VP8,payload=101"),
            nullptr);
    }

    appsink = m_p->makeElement("appsink");
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

    GstPad *src_pad = gst_element_get_static_pad(m_p->appsrc, "src");
    GstPad *recv_rtp_sink_pad =
        gst_element_request_pad_simple(rtpbin, "recv_rtp_sink_%u");

    gst_bin_add_many(GST_BIN(m_p->pipeline), m_p->appsrc, rtpbin, depayloader,
                     decoder, convert, sink, appsink, nullptr);

    gst_pad_link(src_pad, recv_rtp_sink_pad);
    gst_element_link_pads(rtpbin, "send_rtcp_src_%u", appsink, "sink");
    if (!gst_element_link_many(depayloader, decoder, convert, sink, nullptr)) {
        fmt::print(stderr, "[gst-receiver-{}] failed to link the elements\n",
                   media_type);
        return;
    }

    if (synchronize) {
        m_p->setupClock(clock.address, clock.port);
        /* Set this high enough so that it's higher than the minimum latency
         * on all receivers */

        long latency = delay.count() * GST_MSECOND;

        fmt::print("[gst-receiver-{}] reciever latency {} ns\n", media_type,
                   latency);

        gst_pipeline_set_latency(GST_PIPELINE(m_p->pipeline), latency);
    }
}

media::Receiver::~Receiver() {}

void media::Receiver::start() { m_p->start(); }

void media::Receiver::stop() { m_p->stop(); }

bool media::Receiver::push(const uint8_t *payload, size_t len) {
    GstState cur_state = GST_STATE_NULL, pending_state;
    GstBuffer *buf;
    GstMapInfo info;
    GstFlowReturn ret;

    gst_element_get_state(m_p->pipeline, &cur_state, &pending_state, 0);
    if (cur_state != GST_STATE_PAUSED && cur_state != GST_STATE_PLAYING)
        return true;

    buf = gst_buffer_new_allocate(nullptr, len, nullptr);
    if (buf == nullptr) {
        fmt::print(stderr, "[gst-receiver-{}] Failed to allocate buffer\n",
                   media_type);
        return false;
    }

    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        fmt::print(stderr, "[gst-receiver-{}] Failed to get buffer map\n",
                   media_type);
        return false;
    }
    memcpy(info.data, payload, len);
    gst_buffer_unmap(buf, &info);

    g_signal_emit_by_name(m_p->appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        fmt::print(stderr, "[gst-receiver-{}] Failed to do 'push-buffer'\n",
                   media_type);
        return false;
    }

    return true;
}
