/*
    name rbping.cc
    desc: Reboot By IP packet
    type: Linux kernel module
    author: Enric Cecilla
    usage: ping -p "deadbaba" ip
    tested: linux-2.4.26
*/


#define MODULE
#define __KERNEL__


#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/reboot.h>
#include <linux/smp_lock.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/sysrq.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>
#include <linux/string.h>

#include <net/protocol.h>



#define LKM_VERSION "0.1"
#define LKM_NAME "rbping"
#define MAX_LEN_BUF 20

#define DEBUG


/*
 * handler executed when a ip packet with protocol number 163 is received.
*/
int new_rcv(struct sk_buff *);

/*
 * reboot string, dynamically configurable although the module is already loaded
*/
char _magicstr[MAX_LEN_BUF];
char* magicstr;
module_param(magicstr, charp, 0000 );
MODULE_PARM_DESC(magicstr, "Magic string to reboot the system.");


/*
 * /proc filesystem entry for reading/writing module parameter magicstr.  
*/
struct proc_dir_entry* rbping_proc_entry = NULL;

struct net_protocol new_protocol = {
  
    .handler = new_rcv,
};


int
rbping_proc_read( char* buffer, char** buffer_loc, off_t offset, int buflen, int* eof, void* data){

   int ret = 0;
   if( offset == 0 )
      ret = sprintf( buffer, "magicstr:%s\n", _magicstr );

   return ret;
}


int
rbping_proc_write( struct file* file, const char* buffer, unsigned long count, void* data)
{
   
 
  char* wbuffer = buffer;

 
  if( wbuffer[count-1] == '\n' ){

   wbuffer[count-1] = '\0';
   
  }


   if( count > MAX_LEN_BUF )	      
      return -EFAULT;
   


   if( copy_from_user( _magicstr, buffer, count ) )
	return -EFAULT;

   return count;
}



      
int 
new_rcv( struct sk_buff *skb )
{
  
  char databuf[9];
    
  memcpy(databuf,skb->data,9);
  databuf[8]='\0';
 
  if( !strcmp( _magicstr,databuf ) )
    emergency_restart();
    
#ifdef DEBUG

  printk("<1>%s: data rcv: %s\n", LKM_NAME, databuf);
      
#endif
        
  return 0;
}


static int __init rbping_init(void)
{
 
  if( strlen(magicstr) > MAX_LEN_BUF ) {

    printk("rbping_init: error: magicstr too long (%d bytes string permitted)", MAX_LEN_BUF);
    return -1;
  }  
	
  strncpy(_magicstr,magicstr,MAX_LEN_BUF); 

  rbping_proc_entry = create_proc_entry( LKM_NAME, 0644, NULL );
  if( !rbping_proc_entry ){  
    
    remove_proc_entry( LKM_NAME, NULL);
    printk("rbping_init: cannot create /proc entry.");
    return -ENOMEM;
  }
  rbping_proc_entry->read_proc = rbping_proc_read;
  rbping_proc_entry->write_proc = rbping_proc_write;
  
  if ( inet_add_protocol( &new_protocol, 163 ) < 0 ){        
    
    printk("rbping_init: Cannot add ICMP protocol\n");  
    if( inet_del_protocol( &new_protocol, 163 ) < 0 )
      printk("rbping_init: Cannot delete ICMP protocol\n");  
    return -1;
    
  }

  printk("%s-%s: Module loaded\n",LKM_NAME, LKM_VERSION);
  return 0;
  
};

static void __exit rbping_cleanup(void)
{
        
    inet_del_protocol( &new_protocol, 163 );
    remove_proc_entry( LKM_NAME, NULL );
    printk("%s-%s: Module unloaded\n", LKM_NAME, LKM_VERSION);
    
};

module_init(rbping_init);
module_exit(rbping_cleanup);
MODULE_LICENSE("GPL");

