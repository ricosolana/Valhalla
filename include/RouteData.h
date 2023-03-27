#pragma once

#include "VUtils.h"
#include "DataWriter.h"
#include "DataWriter.h"

/*
struct RouteData {
	//OWNER_t m_msgID; // TODO this is never utilized
	OWNER_t m_sender;
	OWNER_t m_target;
	ZDOID m_targetZDO;
	HASH_t m_method;
	//BYTES_t m_params;
	DataReader m_params;
	
	//RouteData() : m_sender(0), m_target(0), m_method(0) {}

	// Will unpack the package
	//RouteData(DataReader &reader) {
	//	//reader.Read<int64_t>(); // msgID
	//	m_sender = reader.Read<OWNER_t>();
	//	m_target = reader.Read<OWNER_t>();
	//	m_targetZDO = reader.Read<ZDOID>();
	//	m_method = reader.Read<HASH_t>();
	//	m_params = reader.Read<BYTES_t>();
	//}

	// Makes no attempts to read the dummy msgid
	//	You must skip that portion first
	RouteData(DataReader& reader) 
	: m_sender(reader.Read<OWNER_t>()),
		m_target(reader.Read<OWNER_t>()),
		m_targetZDO(reader.Read<ZDOID>()),
		m_method(reader.Read<HASH_t>()),
		m_params(reader.SubRead()) {}

	void Serialize(DataWriter &writer) const {
		writer.Write<int64_t>(0);
		writer.Write(m_sender);
		writer.Write(m_target);
		writer.Write(m_targetZDO);
		writer.Write(m_method);
		writer.Write(m_params.m_provider);
	}
};
*/