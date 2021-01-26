#include <stdio.h>
#include <stdlib.h>
#include <string.h> 


/*
 *
 * DESCRIPTION
 * Split each file into multiple substrings
 * using "//@" as delimeter.
 * Merge them alternately.
 * 
 * ex)
 * file_A :
 * --------------------------------
 *  A[0]    int a = 1;
 *          //@
 *  A[1]    printf("b: %d\n",b);
 *          int c = 3;
 *          //@
 *  A[2]    printf("end!\n");
 * --------------------------------
 *
 * file_B :
 * --------------------------------
 *  B[0]    printf("a: %d\n",a);
 *          int b = 2;
 *          //@
 *  B[1]    printf("c: %d\n",c);
 * --------------------------------
 *
 * => result:
 * --------------------------------
 *  A[0]   int a = 1;
 *         //
 *  B[0]   printf("a: %d\n",a);
 *         int b = 2;
 *         //
 *  A[1]   printf("b: %d\n",b);
 *         int c = 3;
 *         //
 *  B[1]   printf("c: %d\n",c);
 *  A[2]   printf("end!\n");
 * --------------------------------
 *
 * USAGE
 * $ gcc merger.c -o merger
 * $ ./merger [fileA] [fileB] [result]
 * 
 */
int main(int argc, char* argv[]) {
    FILE *fileA, *fileB, *result;
    char ch0,ch1;
    int lever;

    fileA = fopen(argv[1],"r");
    fileB = fopen(argv[2],"r");
    result = fopen(argv[3],"w");
    lever = 0;

    if((fileA==NULL)||(fileB==NULL)||(result==NULL)){
        if(fileA==NULL){
            printf("fileA open error\n");
        }else if(fileB==NULL){
            printf("fileB open error\n");
        }else{
            printf("result open error\n");
        }
        return -1;
    }

    while(1){
        if(lever==0){
            ch0 = fgetc(fileA);
            if(ch0==EOF){
                lever=1;
            }else if(ch0=='@'){
                ch0 = fgetc(fileA);
                lever=1;
                fputc('\n',result);
            }else{
                fputc(ch0,result);
            }
        }else{
            ch1 = fgetc(fileB);
            if(ch1==EOF){
                lever=0;
            }else if(ch1=='@'){
                ch1 = fgetc(fileB);
                lever=0;
                fputc('\n',result);
            }else{
                fputc(ch1,result);
            }
        }
        if((ch0==EOF)&&(ch1==EOF)){
            break;
        }
        
    }

    fclose(fileA);
    fclose(fileB);
    fclose(result);
    return 0;
}
