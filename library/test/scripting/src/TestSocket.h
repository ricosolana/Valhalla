#include <exception>
#include <filesystem>
#include <functional>
#include <quill/core/LogLevel.h>
#include <stdexcept>
#include <string_view>

#include <sol/environment.hpp>
#include <sol/forward.hpp>
#include <sol/state.hpp>
#include <sol/table.hpp>
#include <sol/types.hpp>
#include <sol/variadic_args.hpp>
#include <tuple>

#include "Avledet.h"
#include "ModManager.h"
#include "NetSocket.h"
#include "ZDO.h"

class TestSocket : public avledet::network::ISocket
{
  public:
    TestSocket() {}

  public:
    void close(bool linger) override {}

    void send(std::vector<char> bytes) override {}

    std::vector<char> Recv() override
    {
        return {};
    }

    std::string get_host_name() override
    {
        return "host";
    }

    std::string get_address() override
    {
        return "10.10.0.1";
    }

    bool is_outbound() override
    {
        return false;
    }

    Status get_status() override
    {
        return Status::Connected;
    }

    int get_ping() override
    {
        return 21;
    }

    int get_send_queue_size() override
    {
        return 0;
    }

    //std::tuple<float, float, int, float, float> get_connection_stats() override;
    std::tuple<float, float> get_connection_quality() override
    {
        return std::make_tuple(1.f, 1.f);
    }
};
