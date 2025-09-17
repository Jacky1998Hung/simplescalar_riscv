#include<stdio.h>


int main()
{
   FILE *pFile;
   char buffer[] = "Hey";
   pFile = fopen("write.txt", "w");
   if(NULL == pFile) {
       printf("open failure");
       return -1;
   } else {
       fwrite(buffer, 1, sizeof(buffer), pFile);
   }
   fclose(pFile);
   return;
}
