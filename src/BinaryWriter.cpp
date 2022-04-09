#include "BinaryWriter.hpp"
#include "Utils.hpp"

BinaryWriter::BinaryWriter(Stream& stream)
	: m_stream(stream) {}

void BinaryWriter::Write(const byte* in, int offset, int count) {
	m_stream.Write(in, offset, count);
}

void BinaryWriter::Write(const byte* in, int count) {
	Write(in, 0, count);
}

void BinaryWriter::Write(const std::vector<byte>& in, int count) {
	m_stream.Write(in, count);
}

void BinaryWriter::Write(const std::string& in) {
	//value.
	int byteCount = Utils::GetUnicode8Count(in.c_str());
	if (byteCount > 256)
		throw std::runtime_error("Writing big string not yet supported");

	if (byteCount == 0)
		return;

	m_stream.reserveExtra(1 + in.length());

	Write7BitEncodedInt(byteCount);

	Write(reinterpret_cast<const byte*>(in.c_str()), in.length());
}

void BinaryWriter::Write7BitEncodedInt(int in) {
	m_stream.reserveExtra(4);
	unsigned int num;
	for (num = (unsigned int)in; num >= 128U; num >>= 7)
		Write((unsigned char)(num | 128U));

	Write((unsigned char)num);
}
