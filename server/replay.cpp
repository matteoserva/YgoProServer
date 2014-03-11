#include "replay.h"
#include "../ocgcore/ocgapi.h"
#include "../ocgcore/card.h"
#include <algorithm>
#include "lzma/LzmaLib.h"

#define REPLAY_DATA_SIZE 0x20000
namespace ygo {

Replay::Replay() {
	is_recording = false;
	is_replaying = false;
	replay_data = new unsigned char[REPLAY_DATA_SIZE];
	comp_data = replay_data;
	//comp_data = new unsigned char[0x2000];
}
Replay::~Replay() {
	delete [] replay_data;
	//delete [] comp_data;
}
void Replay::BeginRecord() {

	if(is_recording)
		fclose(fp);


	pdata = replay_data;
	is_recording = true;
}
void Replay::WriteHeader(ReplayHeader& header) {
	pheader = header;


}
void Replay::WriteData(const void* data, unsigned int length, bool flush) {
	if(!is_recording)
		return;
    if((pdata+length-replay_data)>=REPLAY_DATA_SIZE)
        return;
	memcpy(pdata, data, length);
	pdata += length;


}
void Replay::WriteInt32(int data, bool flush) {
	if(!is_recording)
		return;
		if((pdata+4-replay_data)>=REPLAY_DATA_SIZE)
        return;
	*((int*)(pdata)) = data;
	pdata += 4;

}
void Replay::WriteInt16(short data, bool flush) {
	if(!is_recording)
		return;
    if((pdata+2-replay_data)>=REPLAY_DATA_SIZE)
        return;
	*((short*)(pdata)) = data;
	pdata += 2;


}
void Replay::WriteInt8(char data, bool flush) {
	if(!is_recording)
		return;
    if((pdata+1-replay_data)>=REPLAY_DATA_SIZE)
        return;
	*pdata = data;
	pdata++;

}
void Replay::Flush() {
	if(!is_recording)
		return;



}
void Replay::EndRecord() {
	if(!is_recording)
		return;


	pheader.datasize = pdata - replay_data;
	/*
	pheader.flag |= REPLAY_COMPRESSED;
	size_t propsize = 5;
	comp_size = 0x1000;
	LzmaCompress(comp_data, &comp_size, replay_data, pdata - replay_data, pheader.props, &propsize, 5, 1 << 24, 3, 0, 2, 32, 1);*/

	//**
	comp_size = pheader.datasize;
	std::swap(comp_data,replay_data);
	//**

	is_recording = false;
}

}
