#include <string>
#include <iostream>
#include <asio.hpp>

#include "NetSocket.h"

ZSocket2::ZSocket2(tcp::socket sock)
	: m_socket(std::move(sock)),
	m_hostname(m_socket.remote_endpoint().address().to_string()) {}

ZSocket2::~ZSocket2() {
	Close();
}



void ZSocket2::Start() {
	LOG(INFO) << "NetSocket2::Start()";

	ReadPkgSize();
}

void ZSocket2::Close() {
	if (Connected()) {
		LOG(INFO) << "NetSocket2::Close()";

		m_connected = false;

		m_socket.close();
	}
}



void ZSocket2::Update() {
	if (m_sendQueue.empty()) {
		WritePkgSize();
	}
}

void ZSocket2::Send(NetPackage::Ptr pkg) {
	if (pkg->GetStream().Length() == 0 || !Connected())
		return;

	auto &&bytes = m_pool.empty() ? bytes_t() : m_pool.pop_front();

	pkg->m_stream.Bytes(bytes);
	m_sendQueueSize += sizeof(m_tempWriteOffset) + bytes.size();
	m_sendQueue.push_back(std::move(bytes)); // capable of blocking
}

NetPackage::Ptr ZSocket2::Recv() {
	if (m_recvQueue.empty())
		return nullptr;
	return m_recvQueue.pop_front();
}



void ZSocket2::ReadPkgSize() {
	auto self(shared_from_this());
	asio::async_read(m_socket,
		asio::buffer(&m_tempReadOffset, sizeof(m_tempReadOffset)),
		[this, self](const std::error_code& e, size_t) {
		if (!e) {
			ReadPkg();
		}
		else {
			LOG(DEBUG) << "read header error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}

void ZSocket2::ReadPkg() {
	if (m_tempReadOffset == 0 || m_tempReadOffset > 10485760) {
		LOG(ERROR) << "Invalid pkg size received " << m_tempReadOffset;
		Close();
	}
	else {		
		auto recv(PKG(m_tempReadOffset));

		auto self(shared_from_this());
		asio::async_read(m_socket,
			asio::buffer(recv->GetStream().Ptr(), m_tempReadOffset), // whether vec needs to be reserved or resized
			[this, self, recv](const std::error_code& e, size_t) {
			if (!e) {
				recv->GetStream().SetLength(m_tempReadOffset);
				m_recvQueue.push_back(recv);
				ReadPkgSize();
			}
			else {
				LOG(DEBUG) << "read body error: " << e.message() << " (" << e.value() << ")";
				Close();
			}
		});
	}
}

void ZSocket2::WritePkgSize() {
	auto &&vec = m_sendQueue.front(); 

	m_tempWriteOffset = vec.size();
	
	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(&m_tempWriteOffset, sizeof(m_tempWriteOffset)),
		[this, self, &vec](const std::error_code& e, size_t) {
		if (!e) {
			m_sendQueueSize -= sizeof(m_tempWriteOffset);
			WritePkg(vec);
		}
		else {
			LOG(DEBUG) << "write header error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}

void ZSocket2::WritePkg(const std::vector<byte_t> &buf) {
	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(buf, m_tempWriteOffset),
		[this, self](const std::error_code& e, size_t) {
		if (!e) {
			auto bytes = m_sendQueue.pop_front();
			m_sendQueueSize -= bytes.size();
			m_pool.push_back(std::move(bytes)); // Return to pool
			if (!m_sendQueue.empty()) {
				WritePkgSize();
			}
		}
		else {
			LOG(DEBUG) << "write body error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}
