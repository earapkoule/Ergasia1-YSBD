#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "heap_file.h"


#define CALL_BF(call)       \
{                           \
  BF_ErrorCode code = call; \
  if (code != BF_OK) {         \
    BF_PrintError(code);    \
    return HP_ERROR;        \
  }                         \
}

HP_ErrorCode HP_Init() {
  return HP_OK;
}

HP_ErrorCode HP_CreateFile(const char *filename) {
  int index, file_desc;
  char* block_data;
  BF_Block *block;

  BF_Block_Init(&block); //Init and allocating mem for the block
  CALL_BF(BF_CreateFile(filename)); //Creating a file
  CALL_BF(BF_OpenFile(filename, &file_desc)); //Opening existing file
  CALL_BF(BF_AllocateBlock(file_desc, block)); //Allocating a block, the first one in this case
  block_data = BF_Block_GetData(block);
  for(index = 0; index < BF_BLOCK_SIZE; index++){
    block_data[index] = '%';
  } //Is a HeapFile only if its first block is filled with '%'
  BF_Block_SetDirty(block); //File was changed
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block); //Free mem
  CALL_BF(BF_CloseFile(file_desc));
  return HP_OK;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc){
  int index;
  char* block_data;
  BF_Block *block;
  BF_Block_Init(&block);

  CALL_BF(BF_OpenFile(fileName, fileDesc));
  CALL_BF(BF_GetBlock(*fileDesc, 0, block));
  block_data = BF_Block_GetData(block);
  for(index = 0; index < BF_BLOCK_SIZE; index++){
    if(block_data[index] != '%'){ //Check if it is a HeapFile
      return HP_ERROR;
    }
  }
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  CALL_BF(BF_CloseFile(fileDesc));
  return HP_OK;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) {
  int blocks_num;
  BF_Block *block;
  BF_Block_Init(&block);
  char *block_data, *block_data_start;
  int records_num;

  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));

  if(blocks_num == 1){ //It has only the block that contains "special" info
    CALL_BF(BF_AllocateBlock(fileDesc, block));
    block_data = BF_Block_GetData(block);
    records_num = 1;
    memcpy(block_data, &records_num, 4);
    block_data = block_data + 4;
    memcpy(block_data, &record, sizeof(Record));
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
  }else{
    CALL_BF(BF_GetBlock(fileDesc, blocks_num - 1, block));
    block_data = BF_Block_GetData(block);
    block_data_start = block_data;
    memcpy(&records_num, block_data, 4);
    block_data = block_data + 4;
    if(records_num < (BF_BLOCK_SIZE - 4)/sizeof(Record)){ //There is space for a record in the same block
      block_data = block_data + (records_num * sizeof(Record));
      memcpy(block_data, &record, sizeof(Record));
      records_num += 1;
      memcpy(block_data_start, &records_num, 4);
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
    }else{
      CALL_BF(BF_UnpinBlock(block));
      CALL_BF(BF_AllocateBlock(fileDesc, block));
      block_data = BF_Block_GetData(block);
      records_num = 1;
      memcpy(block_data, &records_num, 4);
      block_data = block_data + 4;
      memcpy(block_data, &record, sizeof(Record));
      BF_Block_SetDirty(block);
      CALL_BF(BF_UnpinBlock(block));
    }
  }
  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *attrName, void* value) {
  int blocks_num, index1, index2;
  BF_Block *block;
  BF_Block_Init(&block);
  Record *record;
  char *block_data;
  int records_num;
  int *int_value;

  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));
  if(value == NULL){
    for(index1 = 1; index1 < blocks_num; index1++){
      CALL_BF(BF_GetBlock(fileDesc, index1, block));
      block_data = BF_Block_GetData(block);
      memcpy(&records_num, block_data, 4);
      record = (Record*)(block_data + 4);
      for(index2 = 0; index2 < records_num; index2++){
        printf("Id: %d Name: %s Surname: %s City: %s\n", record[index2].id, record[index2].name, record[index2].surname, record[index2].city);
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }else if(!strcmp(attrName, "id")){
    int_value = (int*)value;
    for(index1 = 1; index1 < blocks_num; index1++){
      CALL_BF(BF_GetBlock(fileDesc, index1, block));
      block_data = BF_Block_GetData(block);
      memcpy(&records_num, block_data, 4);
      record = (Record*)(block_data + 4);
      for(index2 = 0; index2 < records_num; index2++){
        if(record[index2].id == *int_value){
          printf("Id: %d Name: %s Surname: %s City: %s\n", record[index2].id, record[index2].name, record[index2].surname, record[index2].city);
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }else if(!strcmp(attrName, "name")){
    value = (char*)value;
    for(index1 = 1; index1 < blocks_num; index1++){
      CALL_BF(BF_GetBlock(fileDesc, index1, block));
      block_data = BF_Block_GetData(block);
      memcpy(&records_num, block_data, 4);
      record = (Record*)(block_data + 4);
      for(index2 = 0; index2 < records_num; index2++){
        if(!strcmp(record[index2].name, value)){
          printf("Id: %d Name: %s Surname: %s City: %s\n", record[index2].id, record[index2].name, record[index2].surname, record[index2].city);
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }else if(!strcmp(attrName, "surname")){
    value = (char*)value;
    for(index1 = 1; index1 < blocks_num; index1++){
      CALL_BF(BF_GetBlock(fileDesc, index1, block));
      block_data = BF_Block_GetData(block);
      memcpy(&records_num, block_data, 4);
      record = (Record*)(block_data + 4);
      for(index2 = 0; index2 < records_num; index2++){
        if(!strcmp(record[index2].surname, value)){
          printf("Id: %d Name: %s Surname: %s City: %s\n", record[index2].id, record[index2].name, record[index2].surname, record[index2].city);
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }else if(!strcmp(attrName, "city")){
    value = (char*)value;
    for(index1 = 1; index1 < blocks_num; index1++){
      CALL_BF(BF_GetBlock(fileDesc, index1, block));
      block_data = BF_Block_GetData(block);
      memcpy(&records_num, block_data, 4);
      record = (Record*)(block_data + 4);
      for(index2 = 0; index2 < records_num; index2++){
        if(!strcmp(record[index2].city, value)){
          printf("Id: %d Name: %s Surname: %s City: %s\n", record[index2].id, record[index2].name, record[index2].surname, record[index2].city);
        }
      }
      CALL_BF(BF_UnpinBlock(block));
    }
  }
  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record) {
  int blocks_num, records_num, max_records; //max_records = Max number of records in a block;
  char *block_data;
  BF_Block *block;
  BF_Block_Init(&block);

  max_records = (BF_BLOCK_SIZE - 4) / sizeof(Record); //The first 4 Bytes are used to store an int indicating the current number of records of the block
  if(rowId <= 0){ //Check if rowId is valid
    return HP_ERROR;
  }
  CALL_BF(BF_GetBlockCounter(fileDesc, &blocks_num));
  if(((rowId - 1) / max_records) + 1 >= blocks_num){ //Check if rowId is in range
    BF_Block_Destroy(&block);
    return HP_ERROR;
  }
  CALL_BF(BF_GetBlock(fileDesc, ((rowId-1)/max_records)+1, block));
  block_data = BF_Block_GetData(block);
  memcpy(&records_num, block_data, 4); //Take the first 4 bytes
  if((rowId - 1) % max_records >= records_num){ //Check if rowId is valid in block
      CALL_BF(BF_UnpinBlock(block));
      BF_Block_Destroy(&block);
      return HP_ERROR;
  }
  memcpy(record, ((block_data + 4) + ((rowId - 1) % max_records) * sizeof(Record)), sizeof(Record)); //Take the record that is written in the rowId position
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;

}
