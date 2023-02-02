#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

#define SET_NUM 64
#define WAY_NUM 8

#define OPEN_FILE_ERR -1
#define WAY_ERR -2
#define ARG_ERR -3
#define MULTIHIT_ERR -4

enum RefillType {
  MISS_RT,
  IPF_RT,
};

struct refillInfo {
  RefillType refillType;
  u_int64_t ext_file_line;
  u_int32_t ptag;
  u_int32_t vidx;
  u_int32_t waymask;
  u_int64_t gtimer;
};

struct metaArrayItem {
  bool valid;
  u_int64_t lastUpdateTime;
  RefillType lastRefillType;
  u_int32_t ptag;
  u_int64_t ext_file_line;
};

std::vector<struct refillInfo*> refillInfos;

struct metaArrayItem metaArray[SET_NUM][WAY_NUM];

int readIn(char* orifile, char* extfile)
{
  FILE* ori_fp = fopen(orifile, "r");
  FILE* ext_fp = fopen(extfile, "w");
  if (NULL == ori_fp || NULL == ext_fp) {
    printf("open file error");
    return OPEN_FILE_ERR;
  }

  char linebuf[500];
  u_int64_t ext_read_line_num = 0;
  int retNum = 0;
  bool isMiss, isPrefetch;
  u_int32_t ptag, vidx, waymask;
  u_int64_t vaddr, gtimer;
  struct refillInfo* pRefillInfo;
  while (fgets(linebuf, 400, ori_fp) != NULL) {
    isMiss = false;
    isPrefetch = false;
    if ((retNum = sscanf(linebuf, "< %ld> MissUnit: write data to meta sram:ptag=0x%x,vidx=0x%x,waymask=0x%x, vaddr=0x%lx", 
                                &gtimer, &ptag, &vidx, &waymask, &vaddr)) == 5) {
      isMiss = true;
    } else if ((retNum = sscanf(linebuf, "< %ld> IPrefetchBuffer: move data to meta sram:ptag=0x%x,vidx=0x%x,waymask=0x%x",
                                &gtimer, &ptag, &vidx, &waymask)) == 4) {
      isPrefetch = true;
    } else {
      continue;
    }

    pRefillInfo = (struct refillInfo*)malloc(sizeof(struct refillInfo));
    if (isMiss) {
      pRefillInfo->refillType = RefillType::MISS_RT;
    } else {
      pRefillInfo->refillType = RefillType::IPF_RT;
    }
    pRefillInfo->ext_file_line = ext_read_line_num;
    pRefillInfo->gtimer = gtimer;
    pRefillInfo->ptag = ptag;
    pRefillInfo->vidx = vidx;
    pRefillInfo->waymask = waymask;
    refillInfos.push_back(pRefillInfo);
  }
  fclose(ori_fp);
  fclose(ext_fp);
  return 0;
}

char* printRefillType(RefillType t)
{
  if (t == RefillType::IPF_RT) {
    return "IPF";
  } else {
    return "MISSUNIT";
  }
}

bool checkMultiHit(FILE* resultFp, struct refillInfo* rInfo)
{
  bool hasMultiHit = false;
  for (int i = 0; i < WAY_NUM; i++) {
    struct metaArrayItem* item = &metaArray[rInfo->vidx][i];
    if (item->valid && item->ptag == rInfo->ptag) {
      hasMultiHit = true;
      fprintf(resultFp, "multiHitTime=%ld, rType=%s, vIdx=0x%x, ptag=0x%x, lastRTime=%ld, lastRType=%s, matchWay=0x%x\n", 
      rInfo->gtimer, printRefillType(rInfo->refillType), rInfo->vidx, rInfo->ptag, item->lastUpdateTime, 
      printRefillType(item->lastRefillType), i);
    }
  }
  return hasMultiHit;
}

int doRefill(struct refillInfo* rInfo)
{
  int way;
  switch (rInfo->waymask)
  {
  case 0x1: way = 0; break;
  case 0x2: way = 1; break;
  case 0x4: way = 2; break;
  case 0x8: way = 3; break;
  case 0x10: way = 4; break;
  case 0x20: way = 5; break;
  case 0x40: way = 6; break;
  case 0x80: way = 7; break;
  default:
    printf("way error\n");
    return WAY_ERR;
  }
  metaArray[rInfo->vidx][way].ext_file_line = rInfo->ext_file_line;
  metaArray[rInfo->vidx][way].lastRefillType = rInfo->refillType;
  metaArray[rInfo->vidx][way].lastUpdateTime = rInfo->gtimer;
  metaArray[rInfo->vidx][way].ptag = rInfo->ptag;
  metaArray[rInfo->vidx][way].valid = true;
  return 0;
}

int simICacheState(char* resultfile)
{
  FILE* resultFp = fopen(resultfile, "w");
  int size = refillInfos.size();
  bool hasMultiHit;
  int r;
  for (struct refillInfo* rInfo : refillInfos) {
    if (checkMultiHit(resultFp, rInfo)) {
      hasMultiHit = true;
    }
    if ((r = doRefill(rInfo)) < 0) {
      return r;
    }
  }
  fclose(resultFp);
  return hasMultiHit ? MULTIHIT_ERR : 0;
}



int main(int argc, char** argv) 
{
  int r;
  if (argc < 4) {
    printf("Usage : ./sim_icache <trace file path> <ext file path> <multi-hit file path>\n");
    return ARG_ERR;
  }
  if ((r = readIn(argv[1], argv[2])) != 0) {
    return r;
  }
  if (r = simICacheState(argv[3]) < 0) {
    return r;
  }
  return 0;
}