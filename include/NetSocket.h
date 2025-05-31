#pragma once

#include <string>
#include <memory>
#include <optional>
#include <queue>
#include <list>

#include <steamnetworkingtypes.h>
#include <isteamfriends.h>

#include "VUtils.h"

#include <vector>
#include <memory>
#include <string>

namespace avledet::network {

    enum class Status {
        //Fresh,
        Connecting,
        Connected,
        Lingering,
        Closed,
        Connect_Failed,
    };

    class Socket : public std::enable_shared_from_this<Socket> {
    public:
        using Ptr = std::shared_ptr<Socket>;

        virtual ~Socket() = default;

        virtual void close(bool linger) = 0;

        virtual std::vector<char> recv() = 0;
        virtual void send(std::vector<char> buf) = 0;

        virtual std::string get_hostname() = 0;
        virtual std::string get_address() = 0;
        virtual bool is_outbound() = 0; // TODO impl

        virtual Status get_status() = 0;
        virtual int get_ping() = 0;
        virtual int get_send_queue_size() = 0;
        //virtual std::tuple<float, float, int, float, float> get_connection_stats() = 0;

        // auto [local, remote] = get_connection_quality()
        virtual std::tuple<float, float> get_connection_quality() = 0;
    };

}// namespace avledet::network
