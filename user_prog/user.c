#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

struct common_data
{
    struct in_addr saddr, daddr;
    unsigned short sport, dport;
};

int main()
{    
    int i, fd, count = 100;    
    struct common_data read_buf;    
    struct in_addr saddr, daddr;
    unsigned short sport, dport;
    size_t len;
    fd = open("/dev/char_dev", O_RDONLY);    
    if (fd == -1) {
        printf("Error in opening file \n");
		return -1;    
    }

    sleep(5);
    while(count --) {
        len = read(fd, &read_buf, 1);
        if(len <= 0) continue;
        printf("%s:%u", inet_ntoa(read_buf.saddr), read_buf.sport);
        printf(" to %s:%u\n", inet_ntoa(read_buf.daddr), read_buf.sport);
    }    
    close(fd);

	return 0;
}
