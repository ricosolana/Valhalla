//#ifdef _WIN32
////#define _WIN32_WINNT 0x0A00
//#define WIN32_LEAN_AND_MEAN
//#endif

#include "ConsoleSSH.h"
#include "Utils.h"

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>

#define USER "myuser"
#define PASSWORD "mypassword"

static bool authenticated = false;
static int tries = 0;
static bool error = false;
static ssh_channel chan = nullptr;

static int auth_none(ssh_session session,
                     const char *user,
                     void *userdata) {
    ssh_string banner = nullptr;

    ssh_set_auth_methods(session,
                         SSH_AUTH_METHOD_PASSWORD | SSH_AUTH_METHOD_GSSAPI_MIC);

    //banner = ssh_string_from_char("Banner Example\n");
    //if (banner != NULL) {ssh_buf
    //    ssh_send_issue_banner(session, banner);
    //}
    //ssh_string_free(banner);

    return SSH_AUTH_DENIED;
}

static int auth_password(ssh_session session, const char *user,
                         const char *password, void *userdata) {
    printf("Authenticating user %s pwd %s\n",user, password);
    if (strcmp(user, USER) == 0 && strcmp(password, PASSWORD) == 0){
        authenticated = true;
        printf("Authenticated\n");
        return SSH_AUTH_SUCCESS;
    }
    if (tries >= 3){
        printf("Too many authentication tries\n");
        ssh_disconnect(session);
        error = true;
        return SSH_AUTH_DENIED;
    }
    tries++;
    return SSH_AUTH_DENIED;
}

static ssh_channel new_session_channel(ssh_session session, void *userdata) {
    if(chan)
        return nullptr;
    printf("Allocated session channel\n");
    chan = ssh_channel_new(session);
    //ssh_callbacks_init(&channel_cb);
    //ssh_set_channel_callbacks(chan, &channel_cb);
    return chan;
}

void sshTest() {
    auto session = ssh_new();
    auto bind = ssh_bind_new();

    struct ssh_server_callbacks_struct cb = {
            .userdata = nullptr,
            .auth_password_function = auth_password,
            .auth_none_function = auth_none,
            .channel_open_request_session_function = new_session_channel
    };

    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_RSAKEY,
                         "ssh_host_rsa_key");

    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_DSAKEY,
                         "ssh_host_dsa_key");

    if (ssh_bind_listen(bind) < 0) {
        LOG(ERROR) << "Unable to SSH listen bind: " << ssh_get_error(bind);
        return;
    }

    if (ssh_bind_accept(bind, session) < 0) {
        LOG(ERROR) << "Unable to accept SSH Connection: " << ssh_get_error(bind);
        return;
    }

    ssh_callbacks_init(&cb);
    ssh_set_server_callbacks(session, &cb);

    if (ssh_handle_key_exchange(session)) {
        LOG(ERROR) << "Failed SSH key exchange: " << ssh_get_error(session);
        return;
    }

    ssh_set_auth_methods(session, SSH_AUTH_METHOD_PASSWORD);
    auto mainloop = ssh_event_new();
    ssh_event_add_session(mainloop, session);



    while (!authenticated && chan) {
        if (error) {
            LOG(ERROR) << "Auth error on main loop";
            break;
        }

        if (ssh_event_dopoll(mainloop, -1) == SSH_ERROR) {
            LOG(ERROR) << ssh_get_error(session);
            ssh_disconnect(session);
            return;
        }
    }

    char buf[2049];

    while (int len = ssh_channel_read(chan, buf, sizeof(buf) - 1, 0)) {
        if (ssh_channel_write(chan, buf, len) == SSH_ERROR) {
            LOG(ERROR) << "error writing to channel\n";
            return;
        }

        buf[len] = '\0';
        LOG(INFO) << "SSh message:" << buf;

        if (buf[0] == '\x0d') {
            if (ssh_channel_write(chan, "\n", 1) == SSH_ERROR) {
                printf("error writing to channel\n");
                return;
            }
        }
    }

    ssh_disconnect(session);
    ssh_bind_free(bind);
    ssh_finalize();
}
