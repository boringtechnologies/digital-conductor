#include <gst/app/app.h>
#include <gst/net/net.h>

#include "core/media/sync.hpp"
#include "internal.hpp"

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
    fmt::print("[gst-{}] {}\n", name, __func__);
    g_object_set(jitterbuffer, "latency", 0, "drop-on-latency", FALSE, NULL);
}

static void actual_sink_added_callback(GstChildProxy *,
                                       GstBaseSink *actual_sink, gchar *,
                                       gpointer) {
    fmt::print("[gst-receiver] {}\n", __func__);

    if (g_object_class_find_property(G_OBJECT_GET_CLASS(G_OBJECT(actual_sink)),
                                     "force-aspect-ratio")) {
        g_object_set(actual_sink, "force-aspect-ratio", TRUE, NULL);
    }
}

struct media::Receiver::impl final : BasePipeline {
    friend class media::Receiver;

    media::WriteCallback write_cb;

    GstElement *appsrc;
    GstElement *rtpbin;

    impl(const std::string &name, media::MediaType type,
         media::WriteCallback write_cb, std::function<void()> onPlayingFn)
        : BasePipeline(name + "-rcv-" + (type ? "video" : "audio"),
                       onPlayingFn),
          write_cb(write_cb),
          appsrc(NULL),
          rtpbin(NULL) {}

    ~impl() override {}

    static GstFlowReturn new_sample(GstAppSink *sink, gpointer userdata) {
        const media::Receiver *self = static_cast<media::Receiver *>(userdata);
        auto pimpl = self->m_p;
        return pimpl->writeSample(sink, pimpl->write_cb, false);
    }

    static void handoff_add_meta(GstElement *identity, GstBuffer *buffer,
                                 gpointer userdata) {
        auto pimpl = static_cast<media::Receiver::impl *>(userdata);
        pimpl->addBufferCreateTimeMeta(identity, buffer);
    }

    static void handoff_handle_meta(GstElement *identity, GstBuffer *buffer,
                                    gpointer userdata) {
        auto pimpl = static_cast<media::Receiver::impl *>(userdata);
        pimpl->measureLatencyForCreateTimeMeta(identity, buffer);
    }
};

media::Receiver::Receiver(const std::string &id, MediaType type,
                          const PlaybackDevice &device,
                          media::WriteCallback write_cb,
                          std::function<void()> onPlayingFn)
    : m_p(new impl(id.c_str(), type, write_cb, onPlayingFn)) {
    GstElement *add_meta_identity, *depayloader, *decoder, *convert,
        *handle_meta_identity, *sink, *appsink;

    const std::string &name_prefix = m_p->name;

    m_p->appsrc = m_p->makeElement("appsrc");
    add_meta_identity = m_p->makeElement(
        "identity", fmt::format("{}-rcv-{}-{}", id, (type ? "video" : "audio"),
                                "add-meta"));
    handle_meta_identity = m_p->makeElement(
        "identity",
        fmt::format("{}-rcv-{}-{}", id, (type ? "video" : "audio"), "handle"));
    g_object_set(m_p->appsrc, "format", GST_FORMAT_TIME, "do-timestamp", true,
                 "is-live", true, nullptr);
    m_p->rtpbin = m_p->makeElement("rtpbin");
    g_signal_connect(m_p->rtpbin, "new-jitterbuffer",
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
        g_signal_connect(add_meta_identity, "handoff",
                         G_CALLBACK(&impl::handoff_add_meta), m_p.get());
        g_signal_connect(handle_meta_identity, "handoff",
                         G_CALLBACK(&impl::handoff_handle_meta), m_p.get());
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
    callbacks.new_sample = &impl::new_sample;
    gst_app_sink_set_callbacks(GST_APP_SINK(appsink), &callbacks, this, NULL);
    g_object_set(appsink, "emit-signals", FALSE, nullptr);

    g_signal_connect(m_p->rtpbin, "pad-added", G_CALLBACK(rtpbin_pad_padded),
                     depayloader);

    GstPad *src_src_pad = gst_element_get_static_pad(m_p->appsrc, "src");
    GstPad *idnt_sink_pad =
        gst_element_get_static_pad(add_meta_identity, "sink");

    GstPad *idnt_src_pad = gst_element_get_static_pad(add_meta_identity, "src");
    GstPad *recv_rtp_sink_pad =
        gst_element_request_pad_simple(m_p->rtpbin, "recv_rtp_sink_%u");

    gst_bin_add_many(GST_BIN(m_p->pipeline), m_p->appsrc, add_meta_identity,
                     m_p->rtpbin, depayloader, decoder, convert,
                     handle_meta_identity, sink, appsink, nullptr);

    gst_pad_link(src_src_pad, idnt_sink_pad);
    gst_pad_link(idnt_src_pad, recv_rtp_sink_pad);
    gst_element_link_pads(m_p->rtpbin, "send_rtcp_src_%u", appsink, "sink");
    if (!gst_element_link_many(depayloader, decoder, convert,
                               handle_meta_identity, sink, nullptr)) {
        fmt::print(stderr, "[gst-{}] failed to link the elements\n", m_p->name);
        return;
    }
}

media::Receiver::~Receiver() {}

void media::Receiver::setSync(bool enabled,
                              std::chrono::duration<int, std::milli> delay,
                              const ClockInfo &clock) {
    // NOTE according to rtpbin implementation buffer-mode & ntp-time-source
    // handled on set_property and no pipeline restart need
    if (enabled) {
        g_object_set(m_p->rtpbin, "ntp-sync", TRUE, "ntp-time-source",
                     3,                 // NTP_TIME_SOURCE_NTP
                     "buffer-mode", 4,  // BUFFER_MODE_SYNCED
                     NULL);
        m_p->setupClock(clock.address, clock.port);
        setSyncDelay(delay);
    } else {
        g_object_set(m_p->rtpbin, "ntp-sync", FALSE, "ntp-time-source",
                     0,                 // NTP_TIME_SOURCE_CLOCK_TIME/
                     "buffer-mode", 0,  // BUFFER_MODE_NONE
                     NULL);
        gst_pipeline_auto_clock(GST_PIPELINE(m_p->pipeline));
        gst_pipeline_set_latency(GST_PIPELINE(m_p->pipeline),
                                 GST_CLOCK_TIME_NONE);
    }
}

void media::Receiver::setSyncDelay(
    std::chrono::duration<int, std::milli> delay) {
    // Set this high enough so that it's higher than the minimum latency on
    // all receivers
    long latency = delay.count() * GST_MSECOND;
    gst_pipeline_set_latency(GST_PIPELINE(m_p->pipeline), latency);
    fmt::print("[gst-{}] reciever latency {} ns\n", m_p->name, latency);
}

void media::Receiver::start() { m_p->start(); }

media::double_millis_t media::Receiver::averageLatency() {
    return double_millis_t(m_p->average_latency);
}

void media::Receiver::stop() { m_p->stop(); }

void media::Receiver::restart() { m_p->restart(); }

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
        fmt::print(stderr, "[gst-{}] Failed to allocate buffer\n", m_p->name);
        return false;
    }

    if (!gst_buffer_map(buf, &info, GST_MAP_WRITE)) {
        fmt::print(stderr, "[gst-{}] Failed to get buffer map\n", m_p->name);
        return false;
    }
    memcpy(info.data, payload, len);
    gst_buffer_unmap(buf, &info);

    g_signal_emit_by_name(m_p->appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        fmt::print(stderr, "[gst-{}] Failed to do 'push-buffer'\n", m_p->name);
        return false;
    }

    return true;
}
