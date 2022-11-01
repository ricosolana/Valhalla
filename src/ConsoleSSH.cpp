//#ifdef _WIN32
////#define _WIN32_WINNT 0x0A00
//#define WIN32_LEAN_AND_MEAN
//#endif

#include <libssh/libssh.h>
#include <libssh/server.h>
#include <libssh/callbacks.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>

#include "ConsoleSSH.h"
#include "Utils.h"

#define USER "myuser"
#define PASSWORD "mypassword"

static bool authenticated = false;
static int tries = 0;
static bool error = false;
static ssh_channel chan = nullptr;

bool generate_key() {
    int				ret = 0;
    RSA				*r = nullptr;
    BIGNUM			*bne = nullptr;
    BIO				*bp_public = nullptr, *bp_private = nullptr;

    int				bits = 2048;
    unsigned long	e = RSA_F4;

    // 1. generate rsa key
    bne = BN_new();
    ret = BN_set_word(bne,e);
    if(ret != 1){
        goto free_all;
    }

    r = RSA_new();
    ret = RSA_generate_key_ex(r, bits, bne, nullptr);
    if(ret != 1){
        goto free_all;
    }

    //bp_public = BIO_new_file("dsa_public.pem", "w+");
    //PEM_write_bio_DSA_PUBKEY()
    //PEM_write_bio_DSAPrivateKey()


    // 2. save public key
    //bp_public = BIO_new_file("rsa_public.pem", "w+");
    //ret = PEM_write_bio_RSAPublicKey(bp_public, r);
    //if(ret != 1){
    //    goto free_all;
    //}

    // 3. save private key
    bp_private = BIO_new_file("rsa_private.pem", "w+");
    ret = PEM_write_bio_RSAPrivateKey(bp_private, r, nullptr, nullptr, 0, nullptr, nullptr);

    // 4. free
    free_all:

    BIO_free_all(bp_public);
    BIO_free_all(bp_private);
    RSA_free(r);
    BN_free(bne);

    return (ret == 1);
}



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
    LOG(INFO) << "Authenticating user " << user << " pwd " << password;
    if (strcmp(user, USER) == 0 && strcmp(password, PASSWORD) == 0){
        authenticated = true;
        LOG(INFO) << "Authenticated";
        return SSH_AUTH_SUCCESS;
    }
    if (tries >= 3){
        LOG(INFO) << "Too many authentication tries";
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
    LOG(INFO) << "Allocated session channel";
    chan = ssh_channel_new(session);
    //ssh_callbacks_init(&channel_cb);
    //ssh_set_channel_callbacks(chan, &channel_cb);
    return chan;
}

void sshTest() {
    generate_key();

    auto session = ssh_new();
    auto bind = ssh_bind_new();

    struct ssh_server_callbacks_struct cb = {
            .userdata = nullptr,
            .auth_password_function = auth_password,
            .auth_none_function = auth_none,
            .channel_open_request_session_function = new_session_channel
    };

    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_RSAKEY,
                         "rsa_private.pem");

    ssh_bind_options_set(bind, SSH_BIND_OPTIONS_DSAKEY,
                         "ssh_host_dsa_key");

    int v = SSH_LOG_TRACE;
    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &v);

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
        if (ssh_channel_write_stderr(chan, buf, len) == SSH_ERROR)
            throw std::runtime_error("error writing to channel");

        buf[len] = '\0';
        LOG(INFO) << "SSH message:" << buf;

        if (buf[0] == '\x0d') {
            if (ssh_channel_write_stderr(chan, "\n", 1) == SSH_ERROR)
                throw std::runtime_error("error writing to channel");
        }
    }

    ssh_disconnect(session);
    ssh_bind_free(bind);
    ssh_finalize();
}
