#include <errno.h>
#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void sigint_cb(pa_mainloop_api* api, pa_signal_event* e, int sig, void* userdata);
void ctx_state_cb(pa_context* c, void* userdata);
void ctx_ready_cb(pa_context* c, const pa_server_info* i, void* userdata);
void ctx_subscribe_cb(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata);
void server_info_cb(pa_context* c, const pa_server_info* i, void* userdata);
void update_awesome(const char* default_sink_name);

int main(int argc, char* argv[]) {

    printf("Starting PA AwesomeWM Widget Utility\n");

    /* create mainloop */
    pa_mainloop* mainloop = pa_mainloop_new();
    if (!mainloop) {
        fprintf(stderr, "pa_mainloop_new() failed\n");
        return EXIT_FAILURE;
    }

    /* get mainloop abstraction layer */
    pa_mainloop_api* mainloop_api = pa_mainloop_get_api(mainloop);
    if (!mainloop_api) {
        fprintf(stderr, "pa_mainloop_get_api() failed\n");
        pa_mainloop_free(mainloop);
        return EXIT_FAILURE;
    }

    /* initialize signal subsystem and bind to mainloop */
    if (pa_signal_init(mainloop_api) != 0) {
        fprintf(stderr, "pa_signal_init() failed\n");
        pa_mainloop_free(mainloop);
        return EXIT_FAILURE;
    }

    /* set callback function to execute on SIGINT */
    pa_signal_event* sigint_evt = pa_signal_new(SIGINT, sigint_cb, NULL);
    if (!sigint_evt) {
        fprintf(stderr, "pa_signal_new() failed\n");
        pa_signal_done();
        pa_mainloop_free(mainloop);
        return EXIT_FAILURE;
    }

    /* create context */
    pa_context* ctx = pa_context_new(mainloop_api, "Awesome Widget Utility");
    if (!ctx) {
        fprintf(stderr, "pa_context_new() failed\n");
        pa_signal_free(sigint_evt);
        pa_signal_done();
        pa_mainloop_free(mainloop);
        return EXIT_FAILURE;
    }

    /*  connect context to server (NULL means default server) */
    if (pa_context_connect(ctx, NULL, 0, NULL) < 0) {
        fprintf(stderr, "pa_context_connect() failed\n");
        pa_context_unref(ctx);
        pa_signal_free(sigint_evt);
        pa_signal_done();
        pa_mainloop_free(mainloop);
        return EXIT_FAILURE;
    }

    /* set callback function to execute on changes to context state */
    pa_context_set_state_callback(ctx, ctx_state_cb, mainloop);

    /* run mainloop and save its status on quit() */
    int retval = EXIT_SUCCESS;
    if (pa_mainloop_run(mainloop, &retval) < 0) {
        fprintf(stderr, "pa_mainloop_run() failed\n");
        pa_context_unref(ctx);
        pa_signal_free(sigint_evt);
        pa_signal_done();
        pa_mainloop_free(mainloop);
        return retval;
    }

    pa_context_unref(ctx);
    pa_signal_free(sigint_evt);
    pa_signal_done();
    pa_mainloop_free(mainloop);

    return retval;
}

/* catch and handle SIGINT */
void sigint_cb(pa_mainloop_api* api, pa_signal_event* e, int sig, void* userdata) {
    exit(EXIT_SUCCESS);
}

/* executes on changes to context state */
void ctx_state_cb(pa_context* c, void* userdata) {

    pa_operation* op;

    pa_context_state_t state = pa_context_get_state(c);

    switch (state) {

        case PA_CONTEXT_READY:

            op = pa_context_get_server_info(c, ctx_ready_cb, userdata);
            if (op) {
                pa_operation_unref(op);
            }

            pa_context_set_subscribe_callback(c, ctx_subscribe_cb, userdata);

            op = pa_context_subscribe(c, PA_SUBSCRIPTION_MASK_SERVER, NULL, userdata);
            if (op) {
                pa_operation_unref(op);
            }
            break;

        default:
            break;
    }
}

/* executes once context is ready */
void ctx_ready_cb(pa_context* c, const pa_server_info* i, void* userdata) {

    printf("Context ready.\n");

    if (i) {
        printf("Default Sink: %s\n", i->default_sink_name);
        printf("Default Source: %s\n", i->default_source_name);
    }
}

/* executes on our context subscription events */
void ctx_subscribe_cb(pa_context* c, pa_subscription_event_type_t t, uint32_t idx, void* userdata) {

    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {

        case PA_SUBSCRIPTION_EVENT_SERVER:
            printf("Server context changed.\n");

            pa_operation* op = pa_context_get_server_info(c, server_info_cb, NULL);
            if (op) {
                pa_operation_unref(op);
            }

            break;
        default:
            fprintf(stderr, "unexpected event type\n");
            break;
    }
}

void server_info_cb(pa_context* c, const pa_server_info* i, void* userdata) {

    if (i) {

        printf("Default Sink: %s\n", i->default_sink_name);
        printf("Default Source: %s\n", i->default_source_name);

        update_awesome(i->default_sink_name);

    } else {
        printf("Error ocurred, not able to get server info!");
    }
}

void update_awesome(const char* default_sink_name) {

    char buffer[512];

    sprintf(buffer, "require('util.audio').update_default_audio_output('%s')", default_sink_name);

    FILE* pipe_child = popen("/usr/bin/awesome-client", "w");

    if (NULL == pipe_child) {
        printf("Failed to create pipe to send command to awesome-client!");
        return;
    }

    fprintf(pipe_child, buffer);

    pclose(pipe_child);
    
}
