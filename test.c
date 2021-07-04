#include <fcntl.h>  
#include <unistd.h>  
#include <sys/mman.h>  
#include <stdio.h>
#include <string.h>
#include<errno.h>
  
#define DEVICE_FILENAME "/dev/mchar"  

struct memory_stat_report
{
	unsigned long vmpeak;
	unsigned long vmsize;
	unsigned long vmlck;
	unsigned long vmpin;
	unsigned long vmhwm;
	unsigned long vmrss;
	unsigned long rssanon;
	unsigned long rssfile;
	unsigned long rssshmem;
	unsigned long vmdata;
	unsigned long vmstk;
	unsigned long vmexe;
	unsigned long vmlib;
	unsigned long vmpte;
	unsigned long vmswap;
	unsigned long hugetlb;
};

  
int main()  
{  
    int fd;  
    int ret;
    char *p = NULL;
    char buff[64];    
  
    fd = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);  

	int pid = getpid();
	ret = write(fd, (char*)(&pid), sizeof(pid));
    if (ret < 0) {
        printf("write error!\n");
        ret = errno;
        goto out;
    }

    ret = read(fd, buff, sizeof(size_t));
    if (ret < 0) {
        printf("read error!\n");
        ret = errno;
        goto out;
    }
	printf("mypid is: %d\n", getpid());
    printf("read: %ld\n", *((size_t*)(&buff[0])));
  
    if(fd >= 0) { 
        p = (char*)mmap(0,  
                4096,  
                PROT_READ | PROT_WRITE,  
                MAP_SHARED,  
                fd,  
                0);  
		struct memory_stat_report* msr = (struct memory_stat_report*)p;
		printf("VmPeak:\t%8lu kB\n" \
			"VmSize:\t%8lu kB\n" \
			"VmLck:\t%8lu kB\n" \
			"VmPin:\t%8lu kB\n" \
			"VmHWM:\t%8lu kB\n" \
			"VmRSS:\t%8lu kB\n" \
			"RssAnon:\t%8lu kB\n" \
			"RssFile:\t%8lu kB\n" \
			"RssShmem:\t%8lu kB\n" \
			"VmData:\t%8lu kB\n" \
			"VmStk:\t%8lu kB\n" \
			"VmExe:\t%8lu kB\n" \
			"VmLib:\t%8lu kB\n" \
			"VmPTE:\t%8lu kB\n" \
			"VmSwap:\t%8lu kB\n" \
			"HugetlbPages:\t%8lu kB\n",
			msr->vmpeak,
			msr->vmsize,
			msr->vmlck,
			msr->vmpin,
			msr->vmhwm,
			msr->vmrss,
			msr->rssanon,
			msr->rssfile,
			msr->rssshmem,
			msr->vmdata,
			msr->vmstk,
			msr->vmexe,
			msr->vmlib,
			msr->vmpte,
			msr->vmswap,
			msr->hugetlb);
        printf("%s", p);  
        munmap(p, 4096);  
  
        close(fd);  
    }  

    //fd = open(DEVICE_FILENAME, O_RDWR|O_NDELAY);

out:
    return ret;  
}  
