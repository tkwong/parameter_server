#include "hdfs/hdfs.h"
#include <iostream>
#include <inttypes.h>

int main(int argc, char **argv) {

    hdfsFS fs = hdfsConnect("proj10", 9000);
   //  const char* writePath = "/testfile.txt";
   //  hdfsFile writeFile = hdfsOpenFile(fs, writePath, O_WRONLY|O_CREAT, 0, 0, 0);
   //  if(!writeFile) {
   //        fprintf(stderr, "Failed to open %s for writing!\n", writePath);
   //        exit(-1);
   //  }
   //  char* buffer = "Hello, World!";
   //  tSize num_written_bytes = hdfsWrite(fs, writeFile, (void*)buffer, strlen(buffer)+1);
   //  if (hdfsFlush(fs, writeFile)) {
   //         fprintf(stderr, "Failed to 'flush' %s\n", writePath);
   //        exit(-1);
   //  }
   // hdfsCloseFile(fs, writeFile);
   
   int num_files;
   const char* readPath = "/datasets/classification/a9/";
   
   hdfsFileInfo* file_info = hdfsListDirectory(fs, readPath, &num_files);
   for (int i = 0; i < num_files; ++i) {
     if (&file_info[i] == nullptr) continue;
    
     if (file_info[i].mKind == kObjectKindFile) {
       // hdfs_block_size = file_info[i].mBlockSize;
       fprintf(stdout, "Name : %s \n",  file_info[i].mName);
       fprintf(stdout, "Size : %ld \n",  (long)file_info[i].mSize);
       fprintf(stdout, "Blocksize : %ld \n",  (long)file_info[i].mBlockSize);
       // return 0;
     }

     // continue;
   }
   hdfsFreeFileInfo(file_info, num_files);
}
