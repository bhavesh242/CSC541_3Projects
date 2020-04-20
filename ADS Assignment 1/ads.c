#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/time.h>

void linear_memory_search(FILE *keyfile, int *seekArr, int *hit, int seek_size, int key_size);
void binary_memory_search(FILE *keyfile, int *seekArr, int *hit, int seek_size,int key_size);
void linear_disk_search(FILE *keyfile, int *seekArr, int *hit, int seek_size,int key_size);
void binary_disk_search(FILE *keyfile, int *seekArr, int *hit, int seek_size,int key_size);
void print_output_and_close_files(FILE *keyfile,FILE *seekfile,int *seekArr,int *hit, int seek_size );

struct  timeval begin,end;

int main(int argc, char* argv[])
{

    if(argc!=4)
    {
        printf("Please Provide correct number of arguments\n");
        exit(1);
    }

    FILE *seekfile,*keyfile;
    int seek_size, key_size;
    int *seekArr, *hit;
    
    //Read the Seek db file
    seekfile = fopen( argv[3], "rb" );
    //Read the Key db File
    keyfile = fopen(argv[2],"rb");
    
    //Get Seek file Size
    fseek(seekfile, 0, SEEK_END); 
    seek_size = ftell(seekfile)/sizeof(int); // get current file pointer
    fseek(seekfile, 0, SEEK_SET);

    //Get Key file Size
    fseek(keyfile, 0, SEEK_END); 
    key_size = ftell(keyfile)/sizeof(int); 
    fseek(keyfile, 0, SEEK_SET);
    
    
    //Allocate Sizes of Seek and Hit Buffers
    seekArr = (int *)malloc(sizeof(int)*seek_size);
    hit = (int *)malloc(sizeof(int)*seek_size);
    
    //Read seek.db data into seek array
    fread(seekArr,sizeof(int),seek_size,seekfile);
    

    if(strcmp(argv[1],"--mem-lin")==0)
    {
        gettimeofday(&begin,NULL);
        linear_memory_search(keyfile,seekArr,hit,seek_size,key_size);
        gettimeofday(&end,NULL);
    }
    else if(strcmp(argv[1],"--mem-bin")==0)
    {
        gettimeofday(&begin,NULL);
        binary_memory_search(keyfile,seekArr,hit,seek_size,key_size);
        gettimeofday(&end,NULL);
    }
    else if(strcmp(argv[1],"--disk-lin")==0)
    {
        gettimeofday(&begin,NULL);
        linear_disk_search(keyfile,seekArr,hit,seek_size,key_size);
        gettimeofday(&end,NULL);
    }
    else if(strcmp(argv[1],"--disk-bin")==0)
    {
        gettimeofday(&begin,NULL);
        binary_disk_search(keyfile,seekArr,hit,seek_size,key_size);
        gettimeofday(&end,NULL);
    }
    else
    {
        printf("Please Provide Proper Arguments\n");
        exit(0);
    }
    

    print_output_and_close_files(keyfile,seekfile,seekArr,hit,seek_size);

    return 0;
    
}

void linear_memory_search(FILE *keyfile, int *seekArr, int *hit, int seek_size, int key_size)
{
    int i,j;
    int *keyArr;

    
    //Allocate Memory to buffers keyArr 
    keyArr = (int *)malloc(sizeof(int)*key_size);

    //Read key file data into key array
    fread(keyArr,sizeof(int),key_size,keyfile);

    //For each integer in seek db, perform a linear search on the keys array 
    for(i=0 ;i<seek_size;i++)
    {
        //Set hit array as zero for each seek value
        hit[i]=0;
        for(j=0;j<key_size;j++)
        {
            //if the seek value is found in keys array, set the hit of that value as 1
            if(keyArr[j]==seekArr[i])
            {
                hit[i]=1;
            }
        }
    }    

}

void binary_memory_search(FILE *keyfile, int *seekArr, int *hit, int seek_size, int key_size)
{
    int cur,left, right,mid;
    int i,j;
    int *keyArr;

    
    //Allocate Memory to buffers keyArr 
    keyArr = (int *)malloc(sizeof(int)*key_size);

    //Read key file data into key array
    fread(keyArr,sizeof(int),key_size,keyfile);

    //For each value in seek array, perform binary search in the key array
    for(i=0;i<seek_size;i++)
    {
        hit[i]=0;
        left =0,right=seek_size-1;
        cur = seekArr[i];
        while(left<=right)
        {
            mid = left + (right-left)/2;
            if(keyArr[mid] == cur)
            {
                hit[i]=1;
                break;
            }    
            else if(keyArr[mid]<cur)
            {
                left = mid+1;
            }
            else
            {
                right = mid-1;
            }
            
        }
    }
}

void linear_disk_search(FILE *keyfile, int *seekArr, int *hit, int seek_size,int key_size)
{
    int i,j,cur;
    
    //For each integer in seek db, perform a linear search on the keys db file 
    for(i=0 ;i<seek_size;i++)
    {
        //Set hit array as zero for each seek value
        hit[i]=0;
        for(j=0;j<key_size;j++)
        {
            fread(&cur,sizeof(int),1,keyfile);
            if(cur==seekArr[i])
            {
                hit[i]=1;
            }
        }
        //Reset pointer to start of the file
        fseek(keyfile, 0, SEEK_SET);
    } 
}
void binary_disk_search(FILE *keyfile, int *seekArr, int *hit, int seek_size,int key_size)
{
  int cur_seek, cur_key,left, right,mid;
    int i,j;

    //For each value in seek array, perform binary search in the key db file 
    for(i=0;i<seek_size;i++)
    {
        hit[i]=0;
        left =0,right=seek_size-1;
        cur_seek = seekArr[i];
        while(left<=right)
        {
            mid = left + (right-left)/2;
            fseek(keyfile,mid*sizeof(int),SEEK_SET);
            fread(&cur_key,sizeof(int),1,keyfile);
            if(cur_key == cur_seek)
            {
                hit[i]=1;
                break;
            }    
            else if(cur_key<cur_seek)
            {
                left = mid+1;
            }
            else
            {
                right = mid-1;
            }
            
        }
        //Reset pointer to start of the file
        fseek(keyfile, 0, SEEK_SET);
    }
}

void print_output_and_close_files(FILE *keyfile,FILE *seekfile,int *seekArr,int *hit, int seek_size)
{
    struct timeval elapsed;
    int secs,usecs;
    int i;
    fclose(keyfile);
    fclose(seekfile);
    for( i=0;i<seek_size;i++)
    {
        if(hit[i]==1)
        {
            printf("%12d: Yes\n",seekArr[i]);
        }
        else
        {
            printf("%12d: No\n",seekArr[i]);
        }
        
    }

	elapsed.tv_sec=end.tv_sec-begin.tv_sec;
	elapsed.tv_usec=end.tv_usec-begin.tv_usec;
	if(elapsed.tv_usec<0){
		elapsed.tv_usec+=1000000;
		elapsed.tv_sec--;
	}

    printf("Time: %ld.%06ld\n",elapsed.tv_sec,elapsed.tv_usec);

}    

