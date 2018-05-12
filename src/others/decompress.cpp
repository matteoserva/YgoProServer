#include <stdio.h>
#include <lzma/LzmaLib.h>
#include <malloc.h>
//#include <stdlib.h>
#define REPLAY_COMPRESSED	0x1
#define REPLAY_TAG			0x2
#define REPLAY_DECODED		0x4

struct ReplayHeader {
	unsigned int id;
	unsigned int version;
	unsigned int flag;
	unsigned int seed;
	unsigned int datasize;
	unsigned int hash;
	unsigned char props[8];
};

int main(int argc, char** argv)
{
if(argc < 2)
	return 1;
FILE* fp = fopen(argv[1], "rb");
if(!fp)
   return 1;

ReplayHeader pheader;
fseek(fp, 0, SEEK_END);
size_t comp_size = ftell(fp) - sizeof(pheader);
fseek(fp, 0, SEEK_SET);

fread(&pheader, sizeof(pheader), 1, fp);
size_t replay_size = pheader.datasize;	

if(0 == (pheader.flag & REPLAY_COMPRESSED)) {
  fclose(fp);
  return 1;
}

unsigned char* replay_data = (unsigned char*)malloc(0x20000);
unsigned char* comp_data = (unsigned char*)malloc(0x2000);
	
fread(comp_data, 0x1000, 1, fp);
fclose(fp);
	
if(LzmaUncompress(replay_data, &replay_size, comp_data, &comp_size, pheader.props, 5) != SZ_OK)
			return 1;
 fp = fopen(argv[1], "wb");
if(!fp)
   return 1;
pheader.flag &= ~REPLAY_COMPRESSED;
fwrite(&pheader,sizeof(pheader),1,fp);
fwrite(replay_data,replay_size,1,fp);


fclose(fp);
free(replay_data);
free(comp_data);

}
