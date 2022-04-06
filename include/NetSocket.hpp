#pragma once

#include <string>
#include "NetPackage.hpp"

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
class Socket2 : public std::enable_shared_from_this<Socket2> {
	tcp::socket m_socket;
	AsyncDeque<Package*> m_sendQueue;
	AsyncDeque<Package*> m_recvQueue;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	std::string m_hostname;
	uint_least16_t m_port;

	//
	std::atomic_bool m_online = true;

public:
	using Ptr = std::shared_ptr<Socket2>;

	Socket2(asio::io_context& ctx);
	~Socket2();

	void Accept();

	/**
		* @brief Send packet
		* @param packet the new Packet()
		* @return result
	*/
	void Send(NOTNULL Package* packet);

	bool HasNewData();
	NULLABLE Package* Recv();
	bool Close();

	std::string& GetHostName();
	uint_least16_t GetHostPort();
	bool IsOnline();
	tcp::socket& GetSocket();

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(Package* pkg);
};
