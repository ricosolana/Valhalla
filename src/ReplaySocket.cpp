#include "NetSocket.h"
#include "VUtilsResource.h"
#include "DataReader.h"
#include "ValhallaServer.h"

ReplaySocket::ReplaySocket(std::string host) {
    // How will be the packet storage format?

    // individual files is inefficient and cluttering

    // im thinking full packets clumped together in 1Mb each?
    
    // depends on avg size of each packet

    // prepare thread to read zstd compressed packets within path
    // path refers to a specific socket by hostname to read

    //m_startTime = steady_clock::now();

    m_thread = std::jthread([this, host](std::stop_token token) {
        size_t chunkIndex = 0;

        const fs::path root = fs::path(VALHALLA_WORLD_RECORDING_PATH) / host;

        while (!token.stop_requested()) {
            if (!fs::exists(root) || !fs::is_directory(root)) {
                LOG(WARNING) << "Failed to find dir " << root.c_str();
                return;
            }

            fs::path path = root / (std::to_string(chunkIndex++) + ".cap");

            bool flag;
            {
                std::scoped_lock<std::mutex> scoped(m_mux);
                flag = m_ready.size() < 1000;
            }

            if (flag) {
                if (auto opt = VUtils::Resource::ReadFile<BYTES_t>(path)) {
                    if (auto decompressed = ZStdDecompressor().Decompress(*opt)) {

                        DataReader reader(opt.value());

                        auto count = reader.Read<int32_t>();
                        for (int i = 0; i < count; i++) {
                            auto ns = nanoseconds(reader.ReadInt64());
                            auto packet = reader.Read<BYTES_t>();

                            std::scoped_lock<std::mutex> scoped(m_mux);
                            m_ready.push_back({ ns, packet });
                        }
                    }
                    else {
                        LOG(WARNING) << "Failed to decompress capture chunk " << path.c_str();
                        return;
                    }

                }
                else {
                    LOG(WARNING) << "Failed to find " << path.c_str() << " or no more packets";
                    return;
                }
            }
            std::this_thread::sleep_for(1ms);
        }
    });

    this->m_originalHost = "REPLAY_" + host;
}

void ReplaySocket::Close(bool flush) {
    m_thread.request_stop();
}

void ReplaySocket::Update() {

}

void ReplaySocket::Send(BYTES_t) {}

std::optional<BYTES_t> ReplaySocket::Recv() {
    std::scoped_lock<std::mutex> scoped(m_mux);
    if (m_ready.empty()) {
        LOG(WARNING) << "No packets queued for replay";
        return std::nullopt;
    }

    auto&& ref = m_ready.front();

    if (Valhalla()->Nanos() >= ref.first) {
        BYTES_t result = std::move(ref.second);
        m_ready.pop_front();
        return result;
    }

    return std::nullopt;
}

std::string ReplaySocket::GetHostName() const { return m_originalHost; }

std::string ReplaySocket::GetAddress() const { return m_originalAddress; }

bool ReplaySocket::Connected() const { return !m_thread.get_stop_token().stop_requested(); }

unsigned int ReplaySocket::GetSendQueueSize() const { return 0; }

unsigned int ReplaySocket::GetPing() const { return 0; }
