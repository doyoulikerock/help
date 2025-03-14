
// test_opus_enc.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>


#include <iostream>
#include <fstream>
#include "OpusEncoder.h"



#pragma pack(push, 1)
struct WavHeader {
	char riff[4]; // "RIFF"
	uint32_t fileSize;
	char wave[4]; // "WAVE"
	char fmt[4]; // "fmt "
	uint32_t fmtSize;
	uint16_t format;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
	char data[4]; // "data"
	uint32_t dataSize;
};
#pragma pack(pop)


unsigned char compressed[8192 * 1024];
int test_wave2opus()
{
	std::ifstream wavFile("d:\\piano2.wav", std::ios::binary);
	if (!wavFile) {
		std::cerr << "Failed to open WAV file." << std::endl;
		return 1;
	}

	WavHeader header;
	wavFile.read(reinterpret_cast<char*>(&header), sizeof(header));
	if (wavFile.gcount() != sizeof(header)) {
		std::cerr << "Failed to read WAV header." << std::endl;
		return 1;
	}

	int sample_rate = header.sampleRate;
	int channels = header.channels;
	int frame_size = 1920; // 40ms at 48kHz

	short* pcm = new short[frame_size * channels];


	try {
		OpusEncoder encoder(sample_rate, channels);
		std::ofstream outfile("d:\\output.opus", std::ios::binary);

		while (wavFile.read(reinterpret_cast<char*>(pcm), frame_size * channels * sizeof(short))) {
			int bytes_encoded = encoder.Encode(pcm, frame_size, compressed, sizeof(compressed));
			if (bytes_encoded) {
				outfile.write(reinterpret_cast<const char*>(compressed), bytes_encoded);

				std::cout << "wrting " << bytes_encoded << "btyes." << std::endl;
			}

		}

		encoder.Flush();
		wavFile.close();
		outfile.close();

		std::cout << "Encoding completed." << std::endl;
	}
	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	delete[] pcm;
	return 0;
}




#include <stdio.h>
#include <stdlib.h>
#include <opus/opus.h>
#include <ogg/ogg.h>
#include <stdint.h>

// WAV 파일 헤더 구조체
typedef struct {
	char riff[4];
	uint32_t chunk_size;
	char wave[4];
	char fmt[4];
	uint32_t subchunk1_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char data[4];
	uint32_t subchunk2_size;
} WAVHeader;

#define BITRATE 128000




// WAV 파일 헤더 작성 함수
void write_wav_header(FILE* fout, WAVHeader* header) {
	fwrite(header, sizeof(WAVHeader), 1, fout);
}

// WAV 파일 헤더 초기화 함수
void init_wav_header(WAVHeader* header, int sample_rate, int num_channels) {
	memcpy(header->riff, "RIFF", 4);
	header->chunk_size = 0;  // 나중에 업데이트
	memcpy(header->wave, "WAVE", 4);
	memcpy(header->fmt, "fmt ", 4);
	header->subchunk1_size = 16;
	header->audio_format = 1;
	header->num_channels = num_channels;
	header->sample_rate = sample_rate;
	header->byte_rate = sample_rate * num_channels * 2;
	header->block_align = num_channels * 2;
	header->bits_per_sample = 16;
	memcpy(header->data, "data", 4);
	header->subchunk2_size = 0;  // 나중에 업데이트
}



int test2_opusenc(){

// 파일 열기
// 파일 열기
FILE* fin;
FILE* fout;
errno_t err_in = fopen_s(&fin, "d:\\piano2.wav", "rb");
errno_t err_out = fopen_s(&fout, "d:\\output.opus", "wb");
if (err_in != 0 || err_out != 0 || !fin || !fout) {
	fprintf(stderr, "Failed to open files\n");
	return 1;
}

// WAV 헤더 읽기
WAVHeader header;
fread(&header, sizeof(WAVHeader), 1, fin);


// 인코더 상태 초기화
int err;
OpusEncoder* encoder = opus_encoder_create(header.sample_rate, header.num_channels, OPUS_APPLICATION_AUDIO, &err);
if (err != OPUS_OK) {
	fprintf(stderr, "Failed to create Opus encoder: %s\n", opus_strerror(err));
	return 1;
}

// 인코딩 설정
opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));



#define FRAME_SIZE		960
// 인코딩 루프
unsigned char cbits[4000];
short *in = new short[FRAME_SIZE * header.num_channels];
while (!feof(fin)) {
	int n = fread(in, sizeof(short), FRAME_SIZE * header.num_channels, fin);
	if (n > 0 ) {
		int nbBytes = opus_encode(encoder, in, n / header.num_channels, cbits, sizeof(cbits));
		if (nbBytes < 0) {
			fprintf(stderr, "Encode failed: %s\n", opus_strerror(nbBytes));
			goto EXIT;
			
		}

		printf("writing ... %d\n", nbBytes);
		fwrite(cbits, 1, nbBytes, fout);
	}
}


EXIT:
// 종료 처리
fclose(fin);
fclose(fout);
opus_encoder_destroy(encoder);









// 디코딩하여 WAV로 저장
FILE* fin_opus;
FILE* fout_wav;
err_in = fopen_s(&fin_opus, "d:\\output.opus", "rb");
err_out = fopen_s(&fout_wav, "d:\\output_decoded.wav", "wb");
if (err_in != 0 || err_out != 0 || !fin_opus || !fout_wav) {
	fprintf(stderr, "Failed to open files\n");
	return 1;
}

// 디코더 상태 초기화
OpusDecoder* decoder = opus_decoder_create(header.sample_rate, header.num_channels, &err);
if (err != OPUS_OK) {
	fprintf(stderr, "Failed to create Opus decoder: %s\n", opus_strerror(err));
	return 1;
}

// WAV 헤더 초기화 및 작성
WAVHeader wav_header;
init_wav_header(&wav_header, header.sample_rate, header.num_channels);
write_wav_header(fout_wav, &wav_header);





// 디코딩 루프
unsigned char cbits_dec[4000];
short *out = new short[FRAME_SIZE * header.num_channels];
int total_samples = 0;
while (!feof(fin_opus)) {
	int nbBytes = fread(cbits_dec, 1, sizeof(cbits_dec), fin_opus);
	if (nbBytes > 0) {
		int frame_size = opus_decode(decoder, cbits_dec, nbBytes, out, FRAME_SIZE, 0);
		if (frame_size < 0) {
			fprintf(stderr, "Decode failed: %s\n", opus_strerror(frame_size));
			return 1;
		}
		fwrite(out, sizeof(short), frame_size * header.num_channels, fout_wav);
		total_samples += frame_size * header.num_channels;
	}
}

// WAV 파일 크기 업데이트
fseek(fout_wav, 4, SEEK_SET);
wav_header.chunk_size = 36 + total_samples * 2;
fwrite(&wav_header.chunk_size, sizeof(uint32_t), 1, fout_wav);
fseek(fout_wav, 40, SEEK_SET);
wav_header.subchunk2_size = total_samples * 2;
fwrite(&wav_header.subchunk2_size, sizeof(uint32_t), 1, fout_wav);

// 종료 처리
fclose(fin_opus);
fclose(fout_wav);
opus_decoder_destroy(decoder);











return 0;


}









int test3_opus_loop() {
	// 파일 열기
	FILE* fin;
	FILE* fout;
	errno_t err_in = fopen_s(&fin, "d:\\piano2.wav", "rb");
	errno_t err_out = fopen_s(&fout, "d:\\output_decoded.wav", "wb");
	if (err_in != 0 || err_out != 0 || !fin || !fout) {
		fprintf(stderr, "Failed to open files\n");
		return 1;
	}

	// WAV 헤더 읽기
	WAVHeader header;
	fread(&header, sizeof(WAVHeader), 1, fin);

	// 인코더 상태 초기화
	int err;
	OpusEncoder* encoder = opus_encoder_create(header.sample_rate, header.num_channels, OPUS_APPLICATION_AUDIO, &err);
	if (err != OPUS_OK) {
		fprintf(stderr, "Failed to create Opus encoder: %s\n", opus_strerror(err));
		return 1;
	}

	// 디코더 상태 초기화
	OpusDecoder* decoder = opus_decoder_create(header.sample_rate, header.num_channels, &err);
	if (err != OPUS_OK) {
		fprintf(stderr, "Failed to create Opus decoder: %s\n", opus_strerror(err));
		return 1;
	}

	// 인코딩 설정
	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));

	// WAV 헤더 초기화 및 작성
	WAVHeader wav_header;
	init_wav_header(&wav_header, header.sample_rate, header.num_channels);
	write_wav_header(fout, &wav_header);

	// 인코딩 및 디코딩 루프
	unsigned char cbits[4000];
	short *in = new short[960 * header.num_channels];
	short *out = new short[960 * header.num_channels];
	int total_samples = 0;

	int total_encoded_bytes = 0;
	while (!feof(fin)) {
		int n = fread(in, sizeof(short), 960 * header.num_channels, fin);
		if (n > 0) {
			int nbBytes = opus_encode(encoder, in, n / header.num_channels, cbits, sizeof(cbits));
			if (nbBytes < 0) {
				fprintf(stderr, "Encode failed: %s\n", opus_strerror(nbBytes));
				break;
			}

			total_encoded_bytes += nbBytes;

			int frame_size = opus_decode(decoder, cbits, nbBytes, out, 960, 0);
			if (frame_size < 0) {
				fprintf(stderr, "Decode failed: %s\n", opus_strerror(frame_size));
				break;
			}



			fwrite(out, sizeof(short), frame_size * header.num_channels, fout);
			total_samples += frame_size * header.num_channels;
		}
	}

	fprintf(stderr, "Total encoded bytes: %d bytes\n", total_encoded_bytes);

	// WAV 파일 크기 업데이트
 	fseek(fout, 4, SEEK_SET);
	wav_header.chunk_size = 36 + total_samples * 2;
	fwrite(&wav_header.chunk_size, sizeof(uint32_t), 1, fout);
	fseek(fout, 40, SEEK_SET);
	wav_header.subchunk2_size = total_samples * 2;
	fwrite(&wav_header.subchunk2_size, sizeof(uint32_t), 1, fout);

	// 종료 처리
	fclose(fin);
	fclose(fout);
	opus_encoder_destroy(encoder);
	opus_decoder_destroy(decoder);

	return 0;
}








int test_save_opus2ogg() {
	// 파일 열기
	FILE* fin;
	FILE* fout;
	errno_t err_in = fopen_s(&fin, "d:\\piano2.wav", "rb");
	errno_t err_out = fopen_s(&fout, "d:\\output.opus", "wb");
	if (err_in != 0 || err_out != 0 || !fin || !fout) {
		fprintf(stderr, "Failed to open files\n");
		return 1;
	}

	// WAV 헤더 읽기
	WAVHeader header;
	fread(&header, sizeof(WAVHeader), 1, fin);

	// Opus 인코더 상태 초기화
	int err;
	OpusEncoder* encoder = opus_encoder_create(header.sample_rate, header.num_channels, OPUS_APPLICATION_AUDIO, &err);
	if (err != OPUS_OK) {
		fprintf(stderr, "Failed to create Opus encoder: %s\n", opus_strerror(err));
		return 1;
	}

	// 인코딩 설정
	opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));

	// Ogg 스트림 초기화
	ogg_stream_state os;
	ogg_stream_init(&os, rand());

	// https://www.rfc-editor.org/rfc/rfc7845.txt

	int pkt_cnt = 0;
	int pre_skip = 512;
	// Opus 헤더 작성
	unsigned char opus_header_data[19];
	opus_header_data[0] = 'O';
	opus_header_data[1] = 'p';
	opus_header_data[2] = 'u';
	opus_header_data[3] = 's';
	opus_header_data[4] = 'H';
	opus_header_data[5] = 'e';
	opus_header_data[6] = 'a';
	opus_header_data[7] = 'd';
	opus_header_data[8] = 1; // 버전
	opus_header_data[9] = header.num_channels; // 채널 수
	opus_header_data[10] = pre_skip & 0xFF; // pre-skip (리틀 엔디언)
	opus_header_data[11] = (pre_skip >> 8) & 0xFF;

	opus_header_data[12] = 0; // output gain (0)
	opus_header_data[13] = 0;
	opus_header_data[14] = 0; // channel mapping (0)
	opus_header_data[15] = 0;
	opus_header_data[16] = 0;
	opus_header_data[17] = 0;
	opus_header_data[18] = 0;

	ogg_packet header_packet;
	header_packet.packet = opus_header_data;
	header_packet.bytes = sizeof(opus_header_data);
	header_packet.b_o_s = 1;
	header_packet.e_o_s = 0;
	header_packet.granulepos = 1;
	header_packet.packetno = pkt_cnt++;


	const char* vendor = "ExampleVendor";
	const char* artist = "ARTIST=Example Artist";
	const char* album =  "ALBUM=Example Album";
	const char* title =  "TITLE=Example Title";

	unsigned char opus_comment_data[512];
	unsigned int offset = 0;

	// "OpusTags" 문자열 복사
	strcpy((char*)opus_comment_data + offset, "OpusTags");
	offset += strlen("OpusTags");

	// 벤더 정보 추가
	int vendor_length = strlen(vendor);
	opus_comment_data[offset++] = (vendor_length & 0xFF);
	opus_comment_data[offset++] = (vendor_length >> 8) & 0xFF;
	opus_comment_data[offset++] = (vendor_length >> 16) & 0xFF;
	opus_comment_data[offset++] = (vendor_length >> 24) & 0xFF;
	strcpy((char*)&opus_comment_data[offset], vendor);
	offset += vendor_length;

	// 댓글 항목 수 추가
	unsigned int comment_count = 3;
	opus_comment_data[offset++] = (comment_count & 0xFF);
	opus_comment_data[offset++] = (comment_count >> 8) & 0xFF;
	opus_comment_data[offset++] = (comment_count >> 16) & 0xFF;
	opus_comment_data[offset++] = (comment_count >> 24) & 0xFF;
#if 0
	// 댓글 항목 추가
	strcpy((char*)opus_comment_data + offset, artist);
	offset += strlen(artist) + 1; // NULL 포함
	strcpy((char*)opus_comment_data + offset, album);
	offset += strlen(album) + 1;
	strcpy((char*)opus_comment_data + offset, title);
	offset += strlen(title) + 1;
#else

	const char* comments[] = { artist, album, title };
	for (int i = 0; i < comment_count; ++i) {
		int comment_length = strlen(comments[i]);
		opus_comment_data[offset++] = (comment_length & 0xFF);
		opus_comment_data[offset++] = (comment_length >> 8) & 0xFF;
		opus_comment_data[offset++] = (comment_length >> 16) & 0xFF;
		opus_comment_data[offset++] = (comment_length >> 24) & 0xFF;
		memcpy(&opus_comment_data[offset], comments[i], comment_length);
		offset += comment_length;
	}

#endif
	// VorbisComment 패킷 생성 및 초기화
	ogg_packet comment_packet;
	comment_packet.packet = opus_comment_data;
	comment_packet.bytes = offset;
	comment_packet.b_o_s = 0;
	comment_packet.e_o_s = 0;
	comment_packet.granulepos = 2;
	comment_packet.packetno = pkt_cnt++;

	// 헤더 패킷 스트림에 추가
	ogg_stream_packetin(&os, &header_packet);
	ogg_stream_packetin(&os, &comment_packet);





	// 인코딩 및 Ogg 패킷 저장 루프
	unsigned char cbits[4000];
	int one_frame_size = 960 * header.num_channels;
	short *in = new short[one_frame_size];
	ogg_packet op;
	long is_eos = 0;
	opus_int64 granule_position = 0; // granulepos의 초기값
	while (!feof(fin) && !is_eos) {
		
		int n = fread(in, sizeof(short), one_frame_size, fin);
		if (n > 0 ) {
			if (n < one_frame_size)
			{
				memset(in + n, 0, one_frame_size - n);
				is_eos = 1;
				n = one_frame_size;
			}
				
			int nbBytes = opus_encode(encoder, in, n / header.num_channels, cbits, sizeof(cbits));
			if (nbBytes < 0) {
				fprintf(stderr, "Encode failed: %s\n", opus_strerror(nbBytes));
				
				ogg_page og;

				while (ogg_stream_flush(&os, &og) != 0) {
					fwrite(og.header, 1, og.header_len, fout);
					fwrite(og.body, 1, og.body_len, fout);
				}
				
				break;
			}

			
			printf("write encoded stream:%d ...\n", nbBytes);
			op.packet = cbits;
			op.bytes = nbBytes;
			op.b_o_s = 0;
			op.e_o_s = is_eos;
			// playable with VLC, but not in window default player
			op.granulepos = granule_position + 3;

			// playable with default window player, but not work in VLC
			//op.granulepos = 0; // 
			
			op.packetno = pkt_cnt++;

			granule_position += 960;

			ogg_stream_packetin(&os, &op);

			ogg_page og;
			while (ogg_stream_pageout(&os, &og) != 0) {
				fwrite(og.header, 1, og.header_len, fout);
				fwrite(og.body, 1, og.body_len, fout);
			}
		}
	}

	// 스트림 종료 처리
	ogg_stream_flush(&os, NULL);
	ogg_stream_clear(&os);
	fclose(fin);
	fclose(fout);
	opus_encoder_destroy(encoder);

	return 0;
}










int main()
{
	//test_wave2opus();
	//test2_opusenc();

	// ok
	//test3_opus_loop();
	
	// partially ok
	test_save_opus2ogg();

    std::cout << "Hello World!\n";
}

// 프로그램 실행: <Ctrl+F5> 또는 [디버그] > [디버깅하지 않고 시작] 메뉴
// 프로그램 디버그: <F5> 키 또는 [디버그] > [디버깅 시작] 메뉴

// 시작을 위한 팁: 
//   1. [솔루션 탐색기] 창을 사용하여 파일을 추가/관리합니다.
//   2. [팀 탐색기] 창을 사용하여 소스 제어에 연결합니다.
//   3. [출력] 창을 사용하여 빌드 출력 및 기타 메시지를 확인합니다.
//   4. [오류 목록] 창을 사용하여 오류를 봅니다.
//   5. [프로젝트] > [새 항목 추가]로 이동하여 새 코드 파일을 만들거나, [프로젝트] > [기존 항목 추가]로 이동하여 기존 코드 파일을 프로젝트에 추가합니다.
//   6. 나중에 이 프로젝트를 다시 열려면 [파일] > [열기] > [프로젝트]로 이동하고 .sln 파일을 선택합니다.
