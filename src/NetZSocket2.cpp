#include <string>
#include <iostream>
#include <asio.hpp>

#include "NetSocket.h"

/*
* If memory allocations from vector is too much of a concern,
* can reuse vectors with some kind of an object maintanance pool
*/

ZSocket2::ZSocket2(tcp::socket sock)
	: m_socket(std::move(sock)),
	m_hostname(m_socket.remote_endpoint().address().to_string()),
	m_port(m_socket.remote_endpoint().port()),
	m_connectivity(Connectivity::CONNECTING) {
}

ZSocket2::~ZSocket2() {
	Close();
}

void ZSocket2::Start() {
	LOG(INFO) << "NetSocket2::Start()";

	m_connectivity = Connectivity::CONNECTED;
	ReadPkgSize();
}

void ZSocket2::Send(NetPackage::Ptr pkg) {
	if (pkg->GetStream().Length() == 0 
		|| m_connectivity == Connectivity::CLOSED)
		return;

	if (pkg->GetStream().Length() + 4 > 10485760) {
		LOG(ERROR) << "Too big package";
		Close();
	} else if (m_connectivity != Connectivity::CLOSED) {
		const bool was_empty = m_sendQueue.empty();

		std::vector<byte_t> buf;
		pkg->GetStream().Bytes(buf);
		m_sendQueue.push_back(std::move(buf)); // this blocks
		m_sendQueueSize += sizeof(m_tempWriteOffset) + buf.size();

		if (was_empty) {
			WritePkgSize();
		}
	}
}

NetPackage::Ptr ZSocket2::Recv() {
	return m_recvQueue.pop_front();
}

bool ZSocket2::HasNewData() {
	return !m_recvQueue.empty();
}

void ZSocket2::Close() {
	if (m_connectivity != Connectivity::CLOSED) {
		LOG(INFO) << "NetSocket2::Close()";

		m_connectivity = Connectivity::CLOSED;

		m_socket.close();
	}
}

std::string& ZSocket2::GetHostName() {
	return m_hostname;
}

uint_least16_t ZSocket2::GetHostPort() {
	return m_port;
}

Connectivity ZSocket2::GetConnectivity() {
	return m_connectivity;
}

int ZSocket2::GetSendQueueSize() {
	return m_sendQueueSize;
}





tcp::socket& ZSocket2::GetSocket() {
	return m_socket;
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
	}
	);
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
			asio::buffer(recv->GetStream().Bytes(), m_tempReadOffset), // whether vec needs to be reserved or resized
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
			m_sendQueueSize -= m_sendQueue.pop_front().size();
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
