#include <asio/ip/tcp.hpp>
#include <cassert>
#include <chrono>
#include <functional>
#include <iostream>
#include <ratio>
#ifdef _WIN32
    #include <iphlpapi.h>
#else
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <sys/socket.h>
#endif

#include "NetSocket.h"
#include "Avledet.h"

namespace avledet::network {

    /*
    * TODO
    * mutex is somewhat redundant
        * a singular known thread runs asio,
        * any (hopeuflly main thread) accesses sockets
    */

    TcpSocket::TcpSocket(asio::ip::tcp::socket socket, asio::ip::tcp::endpoint endpoint, bool is_outbound) :
        m_socket(std::move(socket)),
        m_endpoint(endpoint),
        m_status(Status::Connecting),
        m_is_outbound(is_outbound)
    {
    }

    TcpSocket::~TcpSocket()
    {
        this->close(true);
    }

    void TcpSocket::close(bool linger) noexcept
    {
        if (m_status == Status::Connected && linger) {
            std::scoped_lock shared(m_mux);// unique = "one man shall pass"
            if (!m_send.empty()) {
                m_status = Status::Lingering;

                // https://stackoverflow.com/questions/35154547/modifying-boostasiosocketset-option
                // Flush asio buffers
                m_socket.set_option(asio::socket_base::linger(true, 0));
                return;
            }
        }

        if (m_status != Status::Closed) {
            m_status = Status::Closed;
            std::scoped_lock shared(m_mux);// unique = "one man shall pass"
            m_socket.close();
        }
    }

    void TcpSocket::send(std::vector<char> packet) noexcept
    {
        if (m_status != Status::Connected)
            return;

        assert(!packet.empty() && "dont send empty packets");

        m_send_queue_size += sizeof(m_temp_write_size);
        m_send_queue_size += (std::uint32_t) packet.size();

        std::vector<char> const *ptr_packet {};

        bool was_empty {};
        {
            std::scoped_lock scoped(m_mux);// unique = write-only lock
            was_empty  = m_send.empty();
            m_send.push_back(std::move(packet));
            ptr_packet = &m_send.back();
        }

        // reengage writers
        if (was_empty) {
            assert(ptr_packet);
            this->do_write_header(std::ref(*ptr_packet));
        }
    }

    std::vector<char> TcpSocket::Recv() noexcept
    {
        std::vector<char> result;

        {
            std::scoped_lock shared(m_mux);// unique = write-only lock
            if (!m_recv.empty()) {
                result = std::move(m_recv.front());
                m_recv.pop_front();
            }
        }
        return result;
    }

    std::string TcpSocket::get_host_name() noexcept
    {
        return this->get_address();
    }

    std::string TcpSocket::get_address() noexcept
    {
        //return this->m_address;
        return this->m_endpoint.address().to_string();
    }

    bool TcpSocket::is_outbound() noexcept
    {
        return m_is_outbound;
    }

    Status TcpSocket::get_status() noexcept
    {
        return m_status;
    }

    std::tuple<float, float> TcpSocket::get_connection_quality() noexcept
    {
#ifdef _WIN32
        // TODO
#else
        tcp_info info;
        socklen_t tcp_info_length = sizeof(info);
        auto ret = getsockopt(m_socket.native_handle(), SOL_TCP, TCP_INFO, &info, &tcp_info_length);

        if (ret == 0) {
            //phind.com answer
            //  calculates for the entire stream time. last 3-5 seconds would be better for more immediate quality check
            // https://www.phind.com/search/cmd1pciqh0000206l78htkfsu
            //uint32_t total_packets = info.tcpi_lost + info.tcpi_retrans + info.tcpi_fackets;
            //if (total_packets == 0) return {};
            //
            //// Weight FACK packets differently as they indicate reordering
            //double fack_weight = 0.5;  // Consider FACK packets as half-lost
            //double lost_weight = info->tcpi_lost + (fack_weight * info->tcpi_fackets);
            //
            //return (lost_weight / total_packets) * 100.0;
        }
#endif
        return {};
    }

    int TcpSocket::get_send_queue_size() noexcept
    {
        // TODO use the atomic counting int instead
        //std::shared_lock scoped(m_mux); // shared = read-only lock
        //int size {};
        //for (auto &&packet : m_send) {
        //    size += (int) packet.size();
        //}

#ifdef _WIN32
        // TODO
#else
        tcp_info info;
        socklen_t tcp_info_length = sizeof(info);
        auto ret = getsockopt(m_socket.native_handle(), SOL_TCP, TCP_INFO, &info, &tcp_info_length);

        if (ret == 0) {
            // TODO
            //size += info.tcpi_ + info.tcpi_unacked
        }
#endif

        return (int) m_send_queue_size;
    }

    int TcpSocket::get_ping() noexcept
    {
#ifdef _WIN32
        // https://github.com/notr1ch/TwitchTest/blob/c34fe317f2b7968ab45e585051d1e66273e26049/main.cpp#L709C13-L709C13
        std::vector<char> buf;
        ULONG size = sizeof(MIB_TCPTABLE);
        ULONG res;
        for (;;) {
            buf.resize(size);
            res = GetTcpTable((PMIB_TCPTABLE) buf.data(), &size, TRUE);
            if (res != ERROR_INSUFFICIENT_BUFFER) {
                break;
            }
        }

        if (res != NO_ERROR) {
            return 0;
        }

        PMIB_TCPTABLE tcpTable = (PMIB_TCPTABLE) buf.data();

        asio::error_code ec;
        // https://stackoverflow.com/questions/6716347/not-getting-correct-port-number-by-getextendedtcptable-in-delphi-7
        auto localPort  = asio::detail::socket_ops::host_to_network_short(m_socket.local_endpoint(ec).port());
        if (ec) {
            return 0;
        }

        auto remotePort = asio::detail::socket_ops::host_to_network_short(m_socket.remote_endpoint(ec).port());
        if (ec) {
            return 0;
        }

        PMIB_TCPROW row = nullptr;
        for (unsigned i = 0; i < tcpTable->dwNumEntries; i++) {
            PMIB_TCPROW curr = &tcpTable->table[i];

            if (curr->dwLocalPort == localPort && curr->dwRemotePort == remotePort) {
                row = curr;
                break;
            }
        }

        if (row == nullptr)
            return 0;

        TCP_ESTATS_FINE_RTT_ROD_v0 rod;
        res = GetPerTcpConnectionEStats(row, TcpConnectionEstatsFineRtt,
                                        NULL,// skip Rw
                                        0,   // 0 v
                                        0,   // Rw size (0)
                                        NULL, 0, 0, (PUCHAR) &rod, 0, sizeof(rod));

        if (res == NO_ERROR) {
            return (int) ((float) rod.SumRtt / 1000.f);// time in 'us'
        }

#else
        // https://stackoverflow.com/questions/30979301/how-to-set-get-socket-rtt-in-linux-socket-programming
        using namespace std::chrono;

        tcp_info info;
        socklen_t tcp_info_length = sizeof(info);
        auto ret = getsockopt(m_socket.native_handle(), SOL_TCP, TCP_INFO, &info, &tcp_info_length);

        if (ret == 0) {
            return (int) duration_cast<milliseconds>(microseconds(info.tcpi_rtt)).count();
        }
#endif
        return 0;
    }

    void TcpSocket::do_read()
    {
        assert(m_status != Status::Connected);
        m_status = Status::Connected;

        this->do_read_header();
    }

    void TcpSocket::do_read_header()
    {
        auto self(shared_from_this());
        asio::async_read(m_socket, asio::buffer(&m_temp_read_size, sizeof(m_temp_read_size)),
                         [this, self](std::error_code const &ec, size_t) {
                             if (!ec) {
                                 this->do_read_body();
                             } else {
                                 this->close(false);
                             }
                         });
    }

    void TcpSocket::do_read_body()
    {
        // Max of ~10.48 Mb per packet
        if (m_temp_read_size == 0 || m_temp_read_size > 0xA00000) {
            this->close(false);
        } else {
            m_temp_read_bytes.resize(m_temp_read_size);

            auto self(shared_from_this());
            asio::async_read(m_socket, asio::buffer(m_temp_read_bytes),
                             [this, self](std::error_code const &ec, size_t read) {
                                 if (!ec) {
                                     {
                                         assert(read == m_temp_read_size);
                                         (void) read;

                                         std::scoped_lock scoped(m_mux);// unique = write-only lock
                                         m_recv.push_back(std::move(m_temp_read_bytes));
                                     }
                                     this->do_read_header();
                                 } else {
                                     this->close(false);
                                 }
                             });
        }
    }

    /*
    * Send structure btw internal looping send and external app send
    *   - send()
    *       optional entry point
    *
    *   - write_header()
    *       always eventually reached
    *
    *   - write_body()
    *       always eventually reached
    */

    void TcpSocket::do_write_header(std::reference_wrapper<std::vector<char> const> packet)
    {
        m_temp_write_size = static_cast<std::uint32_t>(packet.get().size());

        auto self(shared_from_this());
        asio::async_write(m_socket, asio::buffer(&m_temp_write_size, sizeof(m_temp_write_size)),
                          [this, self, packet](std::error_code const &ec, size_t) {
                              if (!ec) {
                                  m_send_queue_size -= sizeof(m_temp_write_size);
                                  this->do_write_body(packet);
                              } else {
                                  this->close(false);
                              }
                          });
    }

    void TcpSocket::do_write_body(std::reference_wrapper<std::vector<char> const> packet)
    {
        auto self(shared_from_this());
        asio::async_write(m_socket, asio::buffer(packet.get()),
                          [this, self, packet](std::error_code const &ec, size_t) {
                              if (!ec) {
                                  m_send_queue_size -= (std::uint32_t) packet.get().size();
                                  
                                  //assert(m_send_queue_size >= 0);
                                  

                                  std::vector<char> const *next_packet {};
                                  {
                                      std::scoped_lock scoped(m_mux);// unique = write-only lock
                                      assert(m_send.front() == packet.get());
                                      m_send.pop_front();            // write
                                      if (!m_send.empty()) {
                                          // pop prev, continue
                                          next_packet = &m_send.front();
                                      }
                                  }

                                  if (next_packet) {
                                      // Write next packet
                                      this->do_write_header(std::ref(*next_packet));
                                  } else {
                                      if (m_status == Status::Lingering) {
                                          this->close(false);
                                      }
                                  }
                              } else {
                                  this->close(false);
                              }
                          });
    }

}// namespace avledet::network