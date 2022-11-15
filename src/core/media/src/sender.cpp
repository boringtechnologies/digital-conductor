#include <gst/app/app.h>
#include <gst/net/net.h>

#include <chrono>
#include <iterator>
#include <memory>
#include <thread>

#include "core/media/sync.hpp"
#include "gst/app/gstappsink.h"
#include "internal.hpp"

struct media::Sender::impl final : BasePipeline {
    friend class media::Sender;

    GstElement *tee;

    impl(const std::string &name, media::MediaType type)
        : BasePipeline(name + "-snd-" + (type ? "video" : "audio")),
          tee(NULL) {}

    ~impl() override {}

    GstElement *createSourceElement(const media::CaptureDevice &device) {
        fmt::print("[gst-{}] use {}\n", name, device.name);
        GstElement *source = makeElement(device.name.c_str());
        if (!device.params.empty()) {
            fmt::print("[gst-{}] with params:\n", name);
            for (const std::pair<std::string, std::string> &param :
                 device.params) {
                fmt::print("  {} = {}\n", param.first, param.second);
                if (param.first == "device-index" || param.first == "device") {
                    GType srcType = G_TYPE_STRING;

                    GValue targetValue = G_VALUE_INIT;
                    g_object_get_property(G_OBJECT(source), param.first.c_str(),
                                          &targetValue);
                    GType targetType = targetValue.g_type;

                    if (g_value_type_compatible(srcType, targetType)) {
                        g_value_set_string(&targetValue, param.second.c_str());
                    } else if (g_value_type_transformable(srcType,
                                                          targetType)) {
                        GValue srcValue = G_VALUE_INIT;
                        g_value_init(&srcValue, srcType);
                        g_value_set_string(&srcValue, param.second.c_str());

                        g_value_transform(&srcValue, &targetValue);
                    } else if (targetType == G_TYPE_INT) {
                        gint newValue = std::stoi(param.second);
                        g_value_set_int(&targetValue, newValue);
                    } else {
                        const gchar *srcTypeName = g_type_name(srcType);
                        const gchar *targetTypeName = g_type_name(targetType);
                        fmt::print(
                            stderr,
                            "Cannot set {} to {}.{}. Convertion {} -> {} "
                            "impossible\n",
                            param.second, device.name, param.first, srcTypeName,
                            targetTypeName);
                        continue;
                    }

                    g_object_set_property(G_OBJECT(source), param.first.c_str(),
                                          &targetValue);

                    char *contents = g_strdup_value_contents(&targetValue);
                    fmt::print("Setted x.{} =({}){}\n", param.first, contents,
                               g_type_name(targetType));
                    g_free(contents);
                } else {
                    fmt::print(stderr, "[gst-{}] unknown property: {}\n",
                               param.first);
                }
            }
        }

        return source;
    }
};

media::Sender::Sender(const std::string &id, media::MediaType type,
                      const media::CaptureDevice &device,
                      const media::ClockInfo &clock)
    : m_p(new impl(id.c_str(), type)), media_type(type), write_cbs() {
    GstElement *source, *conv, *rate, *scale, *capsf, *encoder, *payloader,
        *queue, *fakesink;

    source = m_p->createSourceElement(device);

    const std::string &name_prefix = m_p->name;

    if (type == media::MediaType::Audio) {
        conv = m_p->makeElement("audioconvert");
        rate = m_p->makeElement("audioresample");
        capsf = m_p->makeElement("capsfilter");
        g_object_set(capsf, "caps",
                     gst_caps_from_string("audio/x-raw,rate=48000"), nullptr);
        encoder = m_p->makeElement("vorbisenc");
        payloader = gst_element_factory_make("rtpvorbispay",
                                             (id + "-audiopayload").c_str());
        g_object_set(payloader, "pt", 97, "config-interval", 5, NULL);
    } else if (type == media::MediaType::Video) {
        conv = m_p->makeElement("videoconvert");
        rate = m_p->makeElement("videorate");
        scale = m_p->makeElement("videoscale");
        capsf = m_p->makeElement("capsfilter");
        g_object_set(capsf, "caps",
                     gst_caps_from_string(
                         "video/x-raw,width=640,height=480,framerate=30/1"),
                     nullptr);
        encoder = m_p->makeElement("vp8enc");
        // Generates a keyframe every 5s, with RTCP feedback in place
        g_object_set(encoder, "deadline", 1, "keyframe-max-dist", 150,
                     "bits-per-pixel", 0.1f, nullptr);
        payloader = m_p->makeElement("rtpvp8pay");
        g_object_set(payloader, "pt", 101, NULL);
    }

    m_p->tee = m_p->makeElement("tee");
    queue = m_p->makeElement("queue");
    fakesink = m_p->makeElement("fakesink");

    gboolean link_succeed = FALSE;
    if (type == media::MediaType::Audio) {
        gst_bin_add_many(GST_BIN(m_p->pipeline), source, conv, rate, capsf,
                         encoder, payloader, m_p->tee, queue, fakesink, NULL);

        link_succeed =
            gst_element_link_many(source, conv, rate, capsf, encoder, payloader,
                                  m_p->tee, queue, fakesink, NULL);
    } else if (type == media::MediaType::Video) {
        gst_bin_add_many(GST_BIN(m_p->pipeline), source, conv, rate, scale,
                         capsf, encoder, payloader, m_p->tee, queue, fakesink,
                         NULL);

        link_succeed =
            gst_element_link_many(source, conv, rate, scale, capsf, encoder,
                                  payloader, m_p->tee, queue, fakesink, NULL);
    }

    if (!link_succeed) {
        fmt::print(stderr, "[gst-sender-{}] failed to link the elements\n",
                   m_p->name);
        return;
    }

    fmt::print("[gst-sender-{}] elements linked successfully\n", m_p->name);

    m_p->setupClock(clock.address, clock.port);
}

media::Sender::~Sender() {}

void media::Sender::start() { m_p->start(); }

void media::Sender::stop() { m_p->stop(); }

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
    fmt::print("[gst-sender] {} pad added\n", padname);
    if (g_str_has_prefix(padname, prefix)) {
        GstElement *downstream = (GstElement *)userdata;
        GstPad *sinkpad = gst_element_request_pad_simple(downstream, "sink_%u");
        gst_pad_link(pad, sinkpad);
        gst_object_unref(sinkpad);
    }
    g_free(prefix);
    g_free(padname);
}

void media::Sender::addReceiver(const std::string &id,
                                media::WriteCallback write_cb) {
    GstElement *appsink, *rtpbin, *funnel, *queue, *rtcp_appsrc;

    const std::string &name_prefix = m_p->name;
    const std::string bin_name = fmt::format("{}-bin", id);

    GstElement *bin =
        gst_bin_get_by_name(GST_BIN(m_p->pipeline), bin_name.c_str());

    if (bin != nullptr) {
        fmt::print(stderr, "[gst-{}] receiver id {} already exists\n",
                   m_p->name, id);
        return;
    }

    bin = gst_bin_new(bin_name.c_str());
    g_assert(bin != nullptr);

    rtpbin = m_p->makeElement("rtpbin", id);
    funnel = m_p->makeElement("funnel", id);
    queue = m_p->makeElement("queue", id);
    appsink = m_p->makeElement("appsink", id);
    rtcp_appsrc = m_p->makeElement("appsrc", id);

    if (!bin || !funnel || !queue || !rtpbin || !appsink) {
        fmt::print(stderr, "[gst-{}] failed to create elements in addReceiver",
                   m_p->name);
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

    g_object_set(rtcp_appsrc, "format", GST_FORMAT_TIME, nullptr);
    g_object_set(rtcp_appsrc, "do-timestamp", true, nullptr);
    g_object_set(rtcp_appsrc, "is-live", true, nullptr);

    gst_bin_add_many(GST_BIN(bin), rtpbin, funnel, queue, appsink, rtcp_appsrc,
                     nullptr);

    GstPad *rtp_send_sinkpad =
        gst_element_request_pad_simple(rtpbin, "send_rtp_sink_%u");
    if (!gst_element_link_pads(rtpbin, "send_rtcp_src_%u", funnel, "sink_%u"))
        fmt::print(stderr, "[gst-{}] Failed to link rtcp src and funnel\n",
                   m_p->name);

    if (!gst_element_link_pads(rtcp_appsrc, "src", rtpbin, "recv_rtcp_sink_%u"))
        fmt::print(stderr,
                   "[gst-{}] Failed to link rtcp_appsrc and rtcp rcv sink\n",
                   m_p->name);

    if (!gst_element_link_many(funnel, queue, appsink, nullptr))
        fmt::print(stderr, "[gst-{}] failed to link funnel and appsink\n",
                   m_p->name);

    gst_bin_add(GST_BIN(m_p->pipeline), bin);
    gst_element_sync_state_with_parent(bin);

    GstPad *bin_ghost_pad = gst_ghost_pad_new("sink", rtp_send_sinkpad);

    gst_element_add_pad(bin, bin_ghost_pad);
    if (!gst_element_link(m_p->tee, bin))
        fmt::print(stderr, "[gst-{}] failed to link tee and bin\n", m_p->name);

    GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(m_p->pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
                              (m_p->name + "-added-" + id).c_str());
}

struct RemoveCallbackCtx {
    media::Sender *sender;
    std::string id;
    GstElement *bin;
};

gboolean set_to_null(gpointer data) {
    RemoveCallbackCtx *ctx = (RemoveCallbackCtx *)data;

    GstStateChangeReturn ret = gst_element_set_state(ctx->bin, GST_STATE_NULL);
    fmt::print("[gst-sender] state change return {} in {}\n", ret, __func__);

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
        fmt::print(stderr, "[gst-sender] failed to unlink pads in {}\n",
                   __func__);
    }
    gst_object_unref(peer_pad);

    // FIXME: this will NOT be scheduled on our internal mainloop
    g_idle_add(set_to_null, ctx);

    return GST_PAD_PROBE_REMOVE;
}

void media::Sender::removeReceiver(const std::string &id) {
    GstElement *bin = gst_bin_get_by_name(GST_BIN(m_p->pipeline),
                                          fmt::format("{}-bin", id).c_str());
    if (bin == nullptr) {
        fmt::print(stderr,
                   "[gst-{}] no such receiver in the current pipeline for {}\n",
                   m_p->name, id);
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

    gst_element_get_state(m_p->pipeline, &cur_state, &pending_state, 0);
    if (cur_state != GST_STATE_PAUSED && cur_state != GST_STATE_PLAYING)
        return true;

    rtcp_appsrc = gst_bin_get_by_name(GST_BIN(m_p->pipeline),
                                      fmt::format("{}-appsrc", id).c_str());

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

    g_signal_emit_by_name(rtcp_appsrc, "push-buffer", buf, &ret);
    gst_buffer_unref(buf);
    if (ret == GST_FLOW_ERROR) {
        fmt::print(stderr, "[gst-{}] Failed to do 'push-buffer'\n", m_p->name);
        return false;
    }

    return true;
}
