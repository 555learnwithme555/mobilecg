#include "PacketReader.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define LOGD

PacketReader::PacketReader():
	index(0)
{
	buffer.resize(1000);
}

void PacketReader::addByte(char byte) {
	if(packetReady)
		reset();
		
	if(buffer.size() > MAX_BUFFER_SIZE)
		reset();

	if(
		(index == 1 && buffer[0] != 'D') || 
		(index == 2 && buffer[1] != 'A') ||
		(index == 3 && buffer[2] != 'T') ||
		(index == 4 && buffer[3] != 'A') 
	) {
		reset();
	}
	
	if(buffer.size() <= index)
		buffer.resize(index+1000);
	buffer[index] = byte;
	index++;
	packetReady = false;
	
	if(index == sizeof(Packetizer::Header) && !isHeaderOkay()) {
		int oldIndex = index;
		reset();
		lookForMissedPackets(oldIndex);
	}
	
	if(packetReceived()) {
		LOGD("potential packet, length=%d", index);

		if(!isPacketOkay()) {
			LOGD("packet NOT okay");
			int oldIndex = index;
			reset();
			lookForMissedPackets(oldIndex);
		} else {
			LOGD("okay");
			packetReady = true;
		}	
	}

}

bool PacketReader::isPacketReady() {
	return packetReady;
}

Packetizer::Header* PacketReader::getPacketHeader() {
	return (Packetizer::Header*) buffer.data();
}

char* PacketReader::getPacketData() {
	return &buffer[sizeof(Packetizer::Header)];
}

bool PacketReader::isHeaderOkay() {
	Packetizer::Header* header = getPacketHeader();
	uint8_t sum = calcCheckSum(0, sizeof(Packetizer::Header));
	LOGD("packetId %d", header->packetId);
	if(sum != 0)
		return false;
	return true;
}

bool PacketReader::packetReceived() {
	Packetizer::Header* header = getPacketHeader();
	if(index < sizeof(Packetizer::Header))
		return false;
	return index == sizeof(Packetizer::Header) + header->length + 2;
}

static uint8_t pina;
bool PacketReader::isPacketOkay() {
	Packetizer::Header* header = getPacketHeader();
	LOGD("Kurvaanyad: %d", header->packetId);
	int *pinalyuk=(int*)(&buffer[sizeof(Packetizer::Header)+5]);
	LOGD("Bits: %d",*pinalyuk);
	uint16_t sum = -calcCheckSum(sizeof(Packetizer::Header), sizeof(Packetizer::Header) + header->length);
	uint16_t *checksum = (uint16_t*) &buffer[sizeof(Packetizer::Header) + header->length];
	LOGD("checksum calculated: %d, in packet: %d\n", sum, *checksum);
	if(sum != *checksum)
		return false;
	return true;
}

int PacketReader::calcCheckSum(int start, int end) {
	int sum = 0;
	for(int i = start; i < end; ++i)
		sum += (uint8_t)buffer[i];
	return sum;
}

bool PacketReader::isSignatureOkay() {
	Packetizer::Header* header = getPacketHeader();
	if(header->signature != Packetizer::SIGNATURE)
		return false;
	return true;
}

void PacketReader::reset() {
	index = 0;
	packetReady = false;
}

void PacketReader::lookForMissedPackets(int length) {
	for(int i = 1; i < length; ++i)
		addByte(buffer[i]);
}
