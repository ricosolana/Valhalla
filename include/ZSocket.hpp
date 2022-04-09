#pragma once

#include <string>
#include "ZPackage.hpp"

#include "Utils.hpp"
#include <memory>

using namespace asio::ip;

//class ISocket {
//public:
//	virtual ISocket();
//};

/**
	* @brief Custom socket implementation which makes use of Asio tcp socket
	*
*/
// Shared ptr not really necessary with a simple 
// client where socket state is tracked step by step
class ZSocket2 : public std::enable_shared_from_this<ZSocket2> {
	tcp::socket m_socket;

	// If unique ptr is used, the small ptr only is copied
	// but a deallocation will always take place for dead Packages
	// Should use smart ptr anyways to avoid memory leaks
	// ---
	// If ZPackage ref-value is used, the Deque is reallocated to fit the thing
	// A 
	// TODO use unique ptr
	AsyncDeque<ZPackage> m_sendQueue;

	// TODO use unique_ptr
	AsyncDeque<ZPackage> m_recvQueue;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	std::string m_hostname;
	uint_least16_t m_port;

	//
	std::atomic_bool m_online = true;

public:
	using Ptr = std::shared_ptr<ZSocket2>;

	ZSocket2(asio::io_context& ctx);
	~ZSocket2();

	void Accept();

	/**
		* @brief Send packet
		* @param packet the new Packet()
		* @return result
	*/
	void Send(ZPackage packet);

	bool HasNewData();
	ZPackage Recv();
	bool Close();

	std::string& GetHostName();
	uint_least16_t GetHostPort();
	bool IsOnline();
	tcp::socket& GetSocket();

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(ZPackage pkg);
};
