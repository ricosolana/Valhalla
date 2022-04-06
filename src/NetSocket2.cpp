#include "NetSocket.hpp"
#include <string>
#include <iostream>

Socket2::Socket2(asio::io_context& ctx)
	: m_socket(ctx) {}

Socket2::~Socket2() {
	while (!m_sendQueue.empty()) delete m_sendQueue.pop_front();
	while (!m_recvQueue.empty()) delete m_recvQueue.pop_front();

	LOG(DEBUG) << "~NetSocket2()";
	Close();
}



void Socket2::Accept() {
	LOG(DEBUG) << "NetSocket2::Accept()";

	m_hostname = m_socket.remote_endpoint().address().to_string();
	m_port = m_socket.remote_endpoint().port();
	ReadPkgSize();
}


/**
	* In ZSocket2, the packet is sent with a single dispatch
	* the size and data is written to the same buffer
	* Im not doing for readability
*/
void Socket2::Send(NOTNULL Package* pkg) {
	if (pkg->Size() == 0)
		return;

	if (pkg->Size() + 4 > 10485760)
		LOG(ERROR) << "Too big package";

	if (m_online) {
		const bool was_empty = m_sendQueue.empty();
		m_sendQueue.push_back(std::move(pkg));
		if (was_empty) {
			LOG(DEBUG) << "Reinitiating Writer";
			WritePkgSize();
		}
	}
}

bool Socket2::HasNewData() {
	return !m_recvQueue.empty();
}

Package* Socket2::Recv() {
	if (!m_recvQueue.empty())
		return m_recvQueue.pop_front();
	return nullptr;
}



bool Socket2::Close() {
	if (m_online) {
		LOG(DEBUG) << "NetSocket2::Close()";

		m_online = false;

		m_socket.close();

		return true;
	}

	return false;
}



std::string& Socket2::GetHostName() {
	return m_hostname;
}

uint_least16_t Socket2::GetHostPort() {
	return m_port;
}

bool Socket2::IsOnline() {
	return m_online;
}

tcp::socket& Socket2::GetSocket() {
	return m_socket;
}



void Socket2::ReadPkgSize() {
	LOG(DEBUG) << "ReadHeader()";

	auto self(shared_from_this());
	asio::async_read(m_socket,
		asio::buffer(&m_tempReadOffset, 4),
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

void Socket2::ReadPkg() {
	LOG(DEBUG) << "ReadBody()";
			
	if (m_tempReadOffset == 0 || m_tempReadOffset > 10485760) {
		LOG(ERROR) << "Invalid pkg size received " << m_tempReadOffset;
		Close();
	}
	else {
		auto pkg = new Package(m_tempReadOffset);
		//packet->m_buf.resize(packet->offset);
		auto self(shared_from_this());
		asio::async_read(m_socket,
			asio::buffer(pkg->Buffer()),
			[this, self, pkg](const std::error_code& e, size_t) {
			if (!e) {
				m_recvQueue.push_back(pkg);
				ReadPkgSize();
			}
			else {
				LOG(DEBUG) << "read body error: " << e.message() << " (" << e.value() << ")";
				Close();
			}
		});
	}
}

void Socket2::WritePkgSize() {
	LOG(DEBUG) << "WriteHeader()";

	auto pkg = m_sendQueue.front();

	m_tempWriteOffset = pkg->Size();

	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(&m_tempWriteOffset, 4),
		[this, self, pkg](const std::error_code& e, size_t) {
		if (!e) {
			WritePkg(pkg);
		}
		else {
			LOG(DEBUG) << "write header error: " << e.message() << " (" << e.value() << ")";
			Close();
		}
	});
}

void Socket2::WritePkg(Package *pkg) {
	LOG(DEBUG) << "WriteBody()";

	auto self(shared_from_this());
	asio::async_write(m_socket,
		asio::buffer(pkg->Buffer().data(), m_tempWriteOffset),
		[this, self, pkg](const std::error_code& e, size_t) {
		if (!e) {
			delete pkg;
			m_sendQueue.pop_front();
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
