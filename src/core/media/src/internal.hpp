#ifndef MEDIA_INTERNAL_H
#define MEDIA_INTERNAL_H

#include <fmt/core.h>
#include <gst/app/app.h>
#include <gst/gst.h>
#include <gst/net/net.h>

#include <chrono>
#include <mutex>
#include <thread>

#include "core/media/sync.hpp"

#if GST_USE_STATIC_PLUGINS

extern "C" {
// base
GST_PLUGIN_STATIC_DECLARE(coreelements);
GST_PLUGIN_STATIC_DECLARE(coretracers);
GST_PLUGIN_STATIC_DECLARE(app);
GST_PLUGIN_STATIC_DECLARE(audioconvert);
GST_PLUGIN_STATIC_DECLARE(audioresample);
GST_PLUGIN_STATIC_DECLARE(vorbis);
// good
GST_PLUGIN_STATIC_DECLARE(autodetect);
#if GST_CHECK_VERSION(1, 21, 0)
GST_PLUGIN_STATIC_DECLARE(videoconvertscale);
#else
GST_PLUGIN_STATIC_DECLARE(videoconvert);
GST_PLUGIN_STATIC_DECLARE(videoscale);
#endif
GST_PLUGIN_STATIC_DECLARE(videorate);
GST_PLUGIN_STATIC_DECLARE(vpx);
GST_PLUGIN_STATIC_DECLARE(rtp);
GST_PLUGIN_STATIC_DECLARE(rtpmanager);
#if defined(__APPLE__)
GST_PLUGIN_STATIC_DECLARE(osxaudio);
GST_PLUGIN_STATIC_DECLARE(osxvideo);
// bad
GST_PLUGIN_STATIC_DECLARE(applemedia);
#elif defined(__linux__)
GST_PLUGIN_STATIC_DECLARE(alsa);
GST_PLUGIN_STATIC_DECLARE(pulseaudio);
GST_PLUGIN_STATIC_DECLARE(ximagesink);
GST_PLUGIN_STATIC_DECLARE(xvimagesink);
GST_PLUGIN_STATIC_DECLARE(video4linux2);
#endif
GST_PLUGIN_STATIC_DECLARE(libav);
#ifndef NDEBUG
GST_PLUGIN_STATIC_DECLARE(debug);
GST_PLUGIN_STATIC_DECLARE(debugutilsbad);
#endif
}

#endif

static void gst_register_static_plugins() {
#if GST_USE_STATIC_PLUGINS
    GST_PLUGIN_STATIC_REGISTER(coreelements);
    GST_PLUGIN_STATIC_REGISTER(coretracers);
    GST_PLUGIN_STATIC_REGISTER(app);
    GST_PLUGIN_STATIC_REGISTER(audioconvert);
    GST_PLUGIN_STATIC_REGISTER(audioresample);
    GST_PLUGIN_STATIC_REGISTER(vorbis);
    GST_PLUGIN_STATIC_REGISTER(autodetect);
#if GST_CHECK_VERSION(1, 21, 0)
    GST_PLUGIN_STATIC_REGISTER(videoconvertscale);
#else
    GST_PLUGIN_STATIC_REGISTER(videoconvert);
    GST_PLUGIN_STATIC_REGISTER(videoscale);
#endif
    GST_PLUGIN_STATIC_REGISTER(videorate);
    GST_PLUGIN_STATIC_REGISTER(vpx);
    GST_PLUGIN_STATIC_REGISTER(rtp);
    GST_PLUGIN_STATIC_REGISTER(rtpmanager);
#if defined(__APPLE__)
    GST_PLUGIN_STATIC_REGISTER(osxaudio);
    GST_PLUGIN_STATIC_REGISTER(osxvideo);
    GST_PLUGIN_STATIC_REGISTER(applemedia);
#elif defined(__linux__)
    GST_PLUGIN_STATIC_REGISTER(alsa);
    GST_PLUGIN_STATIC_REGISTER(pulseaudio);
    GST_PLUGIN_STATIC_REGISTER(video4linux2);
    GST_PLUGIN_STATIC_REGISTER(ximagesink);
    GST_PLUGIN_STATIC_REGISTER(xvimagesink);
#endif
    GST_PLUGIN_STATIC_REGISTER(libav);
#ifndef NDEBUG
    GST_PLUGIN_STATIC_REGISTER(debug);
    GST_PLUGIN_STATIC_REGISTER(debugutilsbad);
#endif
#endif
}

extern const std::string BUFFER_META_CREATE_TIME;
extern const std::string BUFFER_META_CREATE_TIME_PROP;
const size_t LATENCY_MEASURE_COUNT = 250;

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data);

struct BasePipeline {
    std::string name;
    std::unique_ptr<std::thread> thread;
    std::mutex start_stop_mutex;
    std::function<void()> on_playing_state;
    std::function<void(media::int_millis_t)> on_latency_measured;
    size_t measure_counter;
    double average_latency;
    GMainLoop *loop;
    guint bus_watch_id;
    GstElement *pipeline;

    BasePipeline(
        const std::string &name, std::function<void()> on_playing_state = {},
        std::function<void(media::int_millis_t)> on_latency_measured = {})
        : name(name),
          thread(nullptr),
          start_stop_mutex(),
          on_playing_state(on_playing_state),
          on_latency_measured(on_latency_measured),
          measure_counter(0),
          average_latency(0) {
        g_print("[gst-%s] ctor\n", name.c_str());

        loop = g_main_loop_new(nullptr, FALSE);
        pipeline = gst_pipeline_new(name.c_str());

        GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
        bus_watch_id = gst_bus_add_watch(bus, bus_callback, this);
        gst_object_unref(bus);
    }

    virtual ~BasePipeline() {
        gst_object_unref(GST_OBJECT(pipeline));
        g_source_remove(bus_watch_id);
        g_main_loop_unref(loop);

        g_print("[gst-%s] ~deconstructed\n", name.c_str());
    }

    GstElement *makeElement(const char *factoryname, std::string altname = {}) {
        GstElement *result = gst_element_factory_make(
            factoryname,
            fmt::format("{}-{}", altname.empty() ? name : altname, factoryname)
                .c_str());
        if (!result) {
            fmt::print(stderr, "Failed to find element: {}\n", factoryname);
        }
        return result;
    }

    void setupClock(const std::string &address, int port) {
        GstClock *net_clock =
            gst_net_client_clock_new("net-clock", address.c_str(), port, 0);
        if (net_clock == nullptr) {
            g_print("[gst-%s] failed to create net clock client\n",
                    name.c_str());
            return;
        }

        /* Wait for the clock to stabilise */
        gst_clock_wait_for_sync(net_clock, GST_CLOCK_TIME_NONE);
        gst_pipeline_use_clock(GST_PIPELINE(pipeline), net_clock);
    }

    void addBufferCreateTimeMeta(GstElement *element, GstBuffer *buffer) {
        GstClock *clock = gst_element_get_clock(element);
        GstCustomMeta *meta =
            gst_buffer_add_custom_meta(buffer, BUFFER_META_CREATE_TIME.c_str());
        GstStructure *structure = gst_custom_meta_get_structure(meta);
        gst_structure_set(structure, BUFFER_META_CREATE_TIME_PROP.c_str(),
                          GST_TYPE_CLOCK_TIME, gst_clock_get_time(clock), NULL);
        g_object_unref(clock);
    }

    void measureLatencyForCreateTimeMeta(GstElement *element,
                                         GstBuffer *buffer) {
        GstCustomMeta *meta =
            gst_buffer_get_custom_meta(buffer, BUFFER_META_CREATE_TIME.c_str());
        if (meta) {
            GstClock *clock = gst_element_get_clock(element);
            GstClockTime now = gst_clock_get_time(clock);
            GstClockTime buf_time = 0;
            GstStructure *structure = gst_custom_meta_get_structure(meta);
            gst_structure_get_clock_time(
                structure, BUFFER_META_CREATE_TIME_PROP.c_str(), &buf_time);

            uint64_t latency = GST_TIME_AS_MSECONDS(now - buf_time);
            measure_counter++;
            average_latency +=
                (latency - average_latency) / (double)measure_counter;
            if (measure_counter % LATENCY_MEASURE_COUNT == 0) {
                if (on_latency_measured) {
                    on_latency_measured(
                        media::int_millis_t((long)average_latency));
                }
                measure_counter = 0;
                average_latency = 0;
            }
        }
    }

    GstFlowReturn writeSample(GstAppSink *sink,
                              const media::WriteCallback &write_cb,
                              bool measureLatency = false) {
        if (GstSample *sample = gst_app_sink_pull_sample(sink)) {
            /* The only thing we do in this example is print a * to indicate a
             * received buffer */
            GstBuffer *buf = gst_sample_get_buffer(sample);

            if (measureLatency) {
                measureLatencyForCreateTimeMeta(GST_ELEMENT(sink), buf);
            }

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

    void start() {
        std::lock_guard guard(start_stop_mutex);

        thread = std::make_unique<std::thread>([this] {
            g_print("[gst-%s] Setting the pipeline to playing\n", name.c_str());
            gst_element_set_state(pipeline, GST_STATE_PLAYING);

            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline),
                                      GST_DEBUG_GRAPH_SHOW_ALL,
                                      (name + "-playing").c_str());

            g_print("[gst-%s] Running mainloop\n", name.c_str());
            g_main_loop_run(loop);

            GST_DEBUG_BIN_TO_DOT_FILE(GST_BIN(pipeline),
                                      GST_DEBUG_GRAPH_SHOW_ALL,
                                      (name + "-stopped").c_str());
        });
    }

    void stop() {
        std::lock_guard guard(start_stop_mutex);

        // HACK: FIXME this delay need to avoid deadlock &
        // make sure that thread already called g_main_loop_run
        // ideally something like try_join_for
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(50ms);
        // EOH

        g_main_loop_quit(loop);
        g_print("[gst-%s] stopping playback\n", name.c_str());
        gst_element_set_state(pipeline, GST_STATE_NULL);

        if (thread) {
            thread->join();
            thread.reset();
        }
    }

    void restart() {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
};

static gboolean bus_callback(GstBus *bus, GstMessage *msg, gpointer data) {
    BasePipeline *self = (BasePipeline *)data;
    GMainLoop *loop = (GMainLoop *)self->loop;

    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("end of stream\n");
            g_main_loop_quit(loop);
            break;

        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;

            gst_message_parse_error(msg, &error, &debug);
            g_free(debug);

            g_printerr("error: %s code: %d quark: %d\n", error->message,
                       error->code, error->domain);

            // TODO: handle 'Output window was closed'

            g_error_free(error);
            g_main_loop_quit(loop);
        } break;

        case GST_MESSAGE_STATE_CHANGED: {
            if (!GST_IS_PIPELINE(msg->src)) break;
            GstState old_state, new_state;
            gst_message_parse_state_changed(msg, &old_state, &new_state, NULL);
            if (new_state == GST_STATE_PLAYING && self->on_playing_state) {
                g_print("[gst-%s] GST_MESSAGE_STATE_CHANGED from %s -> %s\n",
                        self->name.c_str(),
                        gst_element_state_get_name(old_state),
                        gst_element_state_get_name(new_state));
                self->on_playing_state();
            }
        } break;
        default:
            // g_printerr("MESSAGE: %s \n", GST_MESSAGE_TYPE_NAME(msg));
            break;
    }

    return TRUE;
}

#endif
