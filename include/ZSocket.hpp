#pragma once

#include <string>
#include "ZPackage.hpp"

#include "Utils.hpp"
#include <memory>

using namespace asio::ip;

class ISocket {
public:
	virtual ~ISocket() = default;

	virtual void Accept() = 0;
	virtual void Send(ZPackage packet) = 0;

	virtual bool HasNewData() = 0;
	virtual ZPackage Recv() = 0;
	virtual bool Close() = 0;

	virtual std::string& GetHostName() = 0;
	virtual uint16_t GetHostPort() = 0;
	virtual bool IsOnline() = 0;
};
/*
class ZSocket3 : public std::enable_shared_from_this<ZSocket3>, ISocket {
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

	std::list<ZPackage> m_list;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	std::atomic_bool m_recv;

	std::string m_hostname;
	uint_least16_t m_port;

	//
	std::atomic_bool m_online = true;

public:
	using Ptr = std::shared_ptr<ZSocket2>;

	ZSocket3(asio::io_context& ctx);
	~ZSocket3();

	void Accept() override;

	void Send(ZPackage packet) override;

	bool HasNewData() override;
	ZPackage Recv() override;
	bool Close() override;

	std::string& GetHostName() override;
	uint_least16_t GetHostPort() override;
	bool IsOnline() override;
	tcp::socket& GetSocket() override;

private:
	void ReadPkgSize();
	void ReadPkg();
	void WritePkgSize();
	void WritePkg(ZPackage& pkg);
};
*/

/**
	* @brief Custom socket implementation which makes use of Asio tcp socket
	*
*/
// Shared ptr not really necessary with a simple 
// client where socket state is tracked step by step
class ZSocket2 : public std::enable_shared_from_this<ZSocket2>, ISocket {
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

	std::list<ZPackage> m_list;

	int m_tempReadOffset = 0;
	int m_tempWriteOffset = 0;

	ZPackage m_recv;

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
	void WritePkg(ZPackage &pkg);
};
