#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <linux/semaphore.h>
#include <linux/cdev.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>

static int Major;

dev_t dev_no, dev;
struct monitor_device
{
	char array[1000][100];
	int windex;
	int rindex;
	struct semaphore sem;
}char_dev;

typedef STRUCT_KFIFO_REC_2(1024) mytest;
static mytest test;

struct common_data
{
    struct in_addr saddr, daddr;
    unsigned short sport, dport;
};

static struct nf_hook_ops nfho;

unsigned int hook_func(const struct nf_hook_ops *ops,
			       struct sk_buff *skb,
			       const struct nf_hook_state *state)
{
    struct iphdr *ip_header = (struct iphdr *)skb_network_header(skb);
    struct udphdr *udp_header;
    /*unsigned int c_addr;
    unsigned int len = 12;
    unsigned short c_port;*/
    //unsigned char buffer[12];
    unsigned int len = 12;
    struct common_data buf;
    //static unsigned long int pkt_count = 0;

    udp_header = (struct udphdr *)skb_transport_header(skb);

    //printk(KERN_INFO"process %ld udp packets\n", ++pkt_count);
    printk(KERN_INFO":  source addr: %pI4:%d destination addr: %pI4:%d\n", &ip_header->saddr, ntohs(udp_header->source), &ip_header->daddr, ntohs(udp_header->dest));
    //if(char_dev.windex >= 1000) char_dev.windex = 0;
    //sprintf(char_dev.array[char_dev.windex], "%d%u%d%u", ip_header->saddr, ntohs(udp_header->source), ip_header->daddr, ntohs(udp_header->dest));
    //sprintf(char_dev.array[char_dev.windex], "source addr: %pI4:%d destination addr: %pI4:%d\n\0", &ip_header->saddr, ntohs(udp_header->source), &ip_header->daddr, ntohs(udp_header->dest));
    //char_dev.windex++;
    
    buf.saddr.s_addr = ip_header->saddr;
    buf.daddr.s_addr = ip_header->daddr;
    printk(KERN_INFO"%d to %d", buf.saddr.s_addr, buf.daddr.s_addr);
    buf.sport = ntohs(udp_header->source);
    buf.dport = ntohs(udp_header->dest);
    

    /*c_addr = ip_header->saddr;
    c_port = ntohs(udp_header->source);
    printk(KERN_INFO"%x %u", c_addr, c_port);
    memcpy(&c_addr, buffer, 4);
    memcpy(&c_port, buffer+4, 2);
    c_addr = ip_header->daddr;
    c_port = ntohs(udp_header->dest);
    printk(KERN_INFO"%x %u\n", c_addr, c_port);
    memcpy(&c_addr, buffer+6, 4);
    memcpy(&c_port, buffer+10, 2);*/
    len = kfifo_in(&test, (char *)&buf, sizeof(struct common_data));

    return NF_ACCEPT;
}

int open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "Inside open \n");
    if(down_interruptible(&char_dev.sem)) {
        printk(KERN_INFO " could not hold semaphore");
        return -1;
  	}
    nfho.hook = (nf_hookfn *)hook_func;
    nfho.hooknum = NF_INET_PRE_ROUTING; // income packet
    nfho.pf = PF_INET; // ipv4
    nfho.priority = NF_IP_PRI_FIRST; // max priority
    nf_register_hook(&nfho);
 	return 0;
}

int release(struct inode *inode, struct file *filp)
{   
    printk (KERN_INFO "Inside close \n");    
    printk(KERN_INFO "Releasing semaphore");   
    up(&char_dev.sem); 
    nf_unregister_hook(&nfho);

    return 0;
}

ssize_t read(struct file *filp, char __user *buff, size_t count, loff_t *offp)
{
    //int rindex = char_dev.rindex;
    //int windex = char_dev.windex;   
    unsigned int copied;
    int ret;
    //if(rindex >= windex) return 0;
    //if(count > strlen(char_dev.array[rindex])) count = strlen(char_dev.array[rindex]);
    //copy_to_user(buff, char_dev.array[rindex], count);
    ret = kfifo_to_user(&test, buff, count*sizeof(struct common_data), &copied);
    //char_dev.rindex ++;   
    return ret ? ret : copied/12;
}

ssize_t write(struct file *filp, const char __user *buff, size_t count, loff_t *offp)
{
       unsigned long ret;
       printk(KERN_INFO "Inside write \n");
       ret = copy_from_user(char_dev.array, buff, count);
       return count;
	return 0;
}

struct file_operations fops = {
        
    .owner = THIS_MODULE,
    .read = read,
    .write = write,
    .open = open,
    .release = release,
};

struct cdev *kernel_cdev;

int char_dev_init (void)
{
      int ret;
      kernel_cdev = cdev_alloc();
      kernel_cdev->ops = &fops;
      kernel_cdev->owner = THIS_MODULE;
      printk (" Inside init module\n");
      ret = alloc_chrdev_region( &dev_no , 0, 1,"chr_arr_dev");
      if (ret < 0) {
      printk("Major number allocation is failed\n");
      return ret;
      }

Major = MAJOR(dev_no);
dev = MKDEV(Major,0);
sema_init(&char_dev.sem,1);
printk (" The major number for your device is %d\n", Major);
ret = cdev_add( kernel_cdev,dev,1);
if(ret < 0 )
{
     printk(KERN_INFO "Unable to allocate cdev");
     return ret;
}
    //char_dev.rindex = char_dev.windex = 0;


    INIT_KFIFO(test);
    return 0;
}

void char_dev_cleanup(void)
{
     printk(KERN_INFO " Inside cleanup_module\n");
     cdev_del(kernel_cdev);
     unregister_chrdev_region(Major, 1);
}

MODULE_LICENSE("GPL");
module_init(char_dev_init);
module_exit(char_dev_cleanup);
