
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

bool is_syncword(uint8_t *media_buf)
{
	if(media_buf[0] == 0xFF && (media_buf[1] & 0xF0) == 0xF0)
		return true;
	else return false;
}

int find_syncword(uint8_t *media_buf)
{
	int i;
	for(i = 0; !is_syncword(&media_buf[i]); ++i);
	return i;
}

uint16_t get_frame_len(uint8_t *media_buf, uint16_t adts_header_size)
{
	uint16_t frame_len;
	if(adts_header_size == 7) {
		frame_len = (uint16_t)((media_buf[3] & 0x03) << 11)
			  + (uint16_t)((media_buf[4] & 0xFF) << 3)
			  + (uint16_t)((media_buf[5] & 0xE0) >> 5)
			  - adts_header_size;
	}
	else if(adts_header_size == 9) {
		frame_len = (uint16_t)((media_buf[5] & 0x03) << 11)
			  + (uint16_t)((media_buf[6] & 0xFF) << 3)
			  + (uint16_t)((media_buf[7] & 0xE0) >> 5)
			  - adts_header_size;
	}
	return frame_len;
}

int main()
{
	char filename[50] = "./Chandelier.aac";
	
	FILE *fp = fopen(filename, "r");
	if(fp == NULL) return false;
	
	struct stat info;
	stat(filename, &info);
	long size = info.st_size;

	uint8_t *buf = calloc(sizeof(uint8_t), size);
	if(buf == NULL) return false;
	
	if (fread(buf, 1, size, fp) != size) 	
		return false;

	uint8_t *media_load = buf;			
	uint16_t adts_header_size = (media_load[1] & 0x01) ? 7 : 9;	
	while(media_load - buf < size) {
		if(!is_syncword(media_load)) 
			media_load += find_syncword(media_load);
		
		uint16_t frame_len = get_frame_len(media_load, adts_header_size);
		
		media_load += adts_header_size;
		printf("[%d]\t", frame_len);
		for(int i = 0; i < frame_len; i++) {
			printf("%.2X ", media_load[i]);
		}
		media_load += frame_len;
		printf("\n");
	}
	
	fclose(fp);
	
	return true;
}