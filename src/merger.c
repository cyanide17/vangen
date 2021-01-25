#include <stdio.h>
#include <stdlib.h>
#include <string.h> 


int main(int argc, char* argv[]) {
    FILE *fs0, *fs1, *ft;
    char ch0,ch1;
    int lever;

    fs0 = fopen(argv[1],"r");
    fs1 = fopen(argv[2],"r");
    ft = fopen(argv[3],"w");
    lever = 0;

    if((fs0==NULL)||(fs1==NULL)||(ft==NULL)){
        if(fs0==NULL){
            printf("fs0 open error\n");
        }else if(fs1==NULL){
            printf("fs1 open error\n");
        }else{
            printf("ft open error\n");
        }
        return -1;
    }

    while(1){
        if(lever==0){
            ch0 = fgetc(fs0);
            if(ch0==EOF){
                lever=1;
            }else if(ch0=='@'){
                ch0 = fgetc(fs0);
                lever=1;
                fputc('\n',ft);
            }else{
                fputc(ch0,ft);
            }
        }else{
            ch1 = fgetc(fs1);
            if(ch1==EOF){
                lever=0;
            }else if(ch1=='@'){
                ch1 = fgetc(fs1);
                lever=0;
                fputc('\n',ft);
            }else{
                fputc(ch1,ft);
            }
        }
        if((ch0==EOF)&&(ch1==EOF)){
            break;
        }
        
    }

    fclose(fs0);
    fclose(fs1);
    fclose(ft);
    return 0;
}
