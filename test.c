#include <fcntl.h>  
#include <unistd.h>  
#include <sys/mman.h>  
#include <stdio.h>
#include <string.h>
#include<errno.h>
  
#define DEVICE_FILENAME "/dev/mchar"  
  
int main()  
{  
    int fd;  
    int ret;
    char *p = NULL;
    char buff[64];    
  
    fd = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);  
  
    if(fd >= 0) { 
        p = (char*)mmap(0,  
                4096,  
                PROT_READ | PROT_WRITE,  
                MAP_SHARED,  
                fd,  
                0);  
        printf("%s", p);  
        munmap(p, 4096);  
  
        close(fd);  
    }  

    fd = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);
	int pid = getpid();
	ret = write(fd, (char*)(&pid), sizeof(pid));
    if (ret < 0) {
        printf("write error!\n");
        ret = errno;
        goto out;
    }

    ret = read(fd, buff, 4);
    if (ret < 0) {
        printf("read error!\n");
        ret = errno;
        goto out;
    }

	printf("mypid is: %d\n", getpid());
    printf("read: %d\n", *((int*)(&buff[0])));
out:
    return ret;  
}  
