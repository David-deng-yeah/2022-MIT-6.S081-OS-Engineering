#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "e1000_dev.h"
#include "net.h"


/*
* tx_ring is an array of `struct tx_desc` objects. Each `struct tx_desc` represents
* a desciptor for transmitting a network packet. These descriptors typically contian
* information about the packet's location in memory, its size, and control flags.
* 
* the tx_ring array is used to manage a circular buffer of descriptors that the network
* card will use for transmitting packets.
* 
* tx_mbufs is an array of pointers to `struct mbuf` objects. An message buffer is
* a data structure to represent a network packet. Each entry in tx_mbufs corresponding
* to a descriptor in the tx_ring. 
* The purpose of tx_mbufs is to associate each descriptor in the tx_ring with the actual
* network packet data that needs to be transmiited.
* 
* Here's a typical sequence of steps 
* for transmitting a network packet using tx_ring and tx_mbufs:
* 1. When a network packet needs to be transmitted,
* an mbuf structure is created to hold the packet data.
* 2. An available descriptor in the tx_ring (e.g., tx_ring[i]) is located. 
* This descriptor will be used to describe how the packet should be transmitted.
* 3. tx_mbufs[i] is set to point to the mbuf containing the packet data. 
* This associates the descriptor in tx_ring with the actual packet data.
* 4. The descriptor in tx_ring is configured with information about the packet, 
* such as its location in memory, length, and control flags. The E1000_TXD_STAT_DD flag, 
* for example, may be cleared to indicate that the descriptor is in use.
* 5. The network driver signals the network card to 
* start transmitting the packet using the descriptor.
*/
#define TX_RING_SIZE 16
static struct tx_desc tx_ring[TX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *tx_mbufs[TX_RING_SIZE];

#define RX_RING_SIZE 16
// the E1000 requires these buffers to be described by an array of descriptor in RAM
// each descriptor contains an address in RAM where the E1000 can write a received packet
// struct rx_desc describes the descriptor format.
// the array of descriptors is called the receive ring, or receive queue.
// it's a circular ring in the sense that when the card or driver reaches the end of the array
// it wraps back to the beginning.
static struct rx_desc rx_ring[RX_RING_SIZE] __attribute__((aligned(16)));
static struct mbuf *rx_mbufs[RX_RING_SIZE];

// remember where the e1000's registers live.
static volatile uint32 *regs;

struct spinlock e1000_lock;


/*
* e1000_init function initializes the E1000 NIC by configuring various
* control registers, setting up transmit and receive descriptor rings,
* configuring interrupt-related settings, and preparing the NIC for packet
* tranmission and reception in the xv6 OS.
*/
// called by pci_init().
// xregs is the memory address at which the
// e1000's registers are mapped.
void
e1000_init(uint32 *xregs)
{
  int i;

  initlock(&e1000_lock, "e1000");

  // register initialization
  // sets the regs pointer to point to the memory-mapped control registers fo the
  // E1000 NIC. These registers are used for configuring and controlling the NIC's
  // behavior
  /*the global variable regs holds a pointer to the E1000's first control register*/
  /*your driver can get at the other registers by indexing regs as an array*/
  /*you'll need to use indices E1000_RDT and E1000_TDT in particular*/
  regs = xregs;

  // Reset the device
  regs[E1000_IMS] = 0; // disable interrupts
  regs[E1000_CTL] |= E1000_CTL_RST;
  regs[E1000_IMS] = 0; // redisable interrupts
  /*
  * __sync_synchronize() ensure that the reset operation is fully completed
  * and that any subsequent operations involving the regs memory-mapped control
  * registers are executed only after the reset hash taken effect. It helps prevent
  * any potential issues that could arise from memory access reordering by the compiler
  * or hardware
  * 
  * quote:
  * 1. Memory Ordering: in multi-threaded or parallel programs, it's important to 
  * maintain a strict memory ordering to ensure that memory operations are observed
  * in a consistent order by all threads or hardware components. Memory barriers like
  * __sync_synchronize() help enforce this ordering.
  * 
  * 2. preventing compiler and hardware optimizations: compilers and hardware may perform
  * various optimizations that could reorder memory operations for performance reasons.
  * However, such reordering might lead to incorrect behavior in programs that rely on
  * a specific order of memory accesses. Memory barriers prevent these optimizations from
  * affecting the order of memory operations
  * 
  * for example, imagine you have two threads A and B, running on separate processor cores,
  * and they both need to access a shared memory location, here's what can happen without
  * proper synchronization:
  * 1) thread A writes a value 42 to a shared memory location, this involves a write operation
  * 2) Meanwhile, thread B wants to read the value from the same shared memory location, However
  * due to various optimizations and the parallel nature of modern processors, thread B's read 
  * operation might not immediately see the value 42 written by thread A
  * 
  * here are a few reasons why Thread B might not immediately see the correct value:
  * a) out-of-order-execution: modern processors may execute instructions out of order for
  * performance reasons. This means that thread B's read operation might be execute before
  * thread A's write operation, even though thread A's write logically happend first
  * 
  * Memory barriers provide a way to enforce a specific order of memory operations, ensuring
  * that writes made by one thread are visible to other threads in the intended order.
  */
  __sync_synchronize();

  // [E1000 14.5] Transmit initialization
  // initializes the transmit(TX) ring, which is used for transmitting packets
  memset(tx_ring, 0, sizeof(tx_ring));
  // clearing and setting the E100_TXD_STAT_DD(Descriptor Done) status bit for
  // each descriptor in the TX ring. Theses descriptors are used to track the 
  // status of packets being transmitted.
  for (i = 0; i < TX_RING_SIZE; i++) {
    // E1000_TXD_STAT_DD is used to indicate whether a particular descriptor
    // is available and ready for a new packet transmission
    tx_ring[i].status = E1000_TXD_STAT_DD;
    tx_mbufs[i] = 0;
  }
  // set register wth the base address of the TX ring
  regs[E1000_TDBAL] = (uint64) tx_ring;
  if(sizeof(tx_ring) % 128 != 0)
    panic("e1000");
  // set register with the size of the TX ring
  regs[E1000_TDLEN] = sizeof(tx_ring);
  // initializes the TX head and tail pointers to 0
  regs[E1000_TDH] = regs[E1000_TDT] = 0;
  
  // [E1000 14.4] Receive initialization
  // the same as transmit 
  memset(rx_ring, 0, sizeof(rx_ring));
  for (i = 0; i < RX_RING_SIZE; i++) {
    // allocates mbuf packet buffers for the E1000 to DMA into
    rx_mbufs[i] = mbufalloc(0);
    if (!rx_mbufs[i])
      panic("e1000");
    rx_ring[i].addr = (uint64) rx_mbufs[i]->head;
  }
  regs[E1000_RDBAL] = (uint64) rx_ring;
  if(sizeof(rx_ring) % 128 != 0)
    panic("e1000");
  regs[E1000_RDH] = 0;
  regs[E1000_RDT] = RX_RING_SIZE - 1;
  regs[E1000_RDLEN] = sizeof(rx_ring);


  // MAC address Filtering
  // it configures MAC address filtering by setting two registers
  // to specify the NIC's MAC address
  // filter by qemu's MAC address, 52:54:00:12:34:56
  regs[E1000_RA] = 0x12005452;
  regs[E1000_RA+1] = 0x5634 | (1<<31);
  // multicast table
  for (int i = 0; i < 4096/32; i++)
    regs[E1000_MTA + i] = 0;


  // Transmitter and Receiver Control Configuration
  // transmitter control bits.
  regs[E1000_TCTL] = E1000_TCTL_EN |  // enable
    E1000_TCTL_PSP |                  // pad short packets
    (0x10 << E1000_TCTL_CT_SHIFT) |   // collision stuff
    (0x40 << E1000_TCTL_COLD_SHIFT);
  regs[E1000_TIPG] = 10 | (8<<10) | (6<<20); // inter-pkt gap
  // receiver control bits.
  regs[E1000_RCTL] = E1000_RCTL_EN | // enable receiver
    E1000_RCTL_BAM |                 // enable broadcast
    E1000_RCTL_SZ_2048 |             // 2048-byte rx buffers
    E1000_RCTL_SECRC;                // strip CRC
  
  // Interrupt configuration
  // it configures interrupt-related registers to specify when the NIC should
  // generate interrupts. For example, it sets up receive interrupt triggers and
  // enable receive descriptor write-back interrupts
  // ask e1000 for receive interrupts.
  regs[E1000_RDTR] = 0; // interrupt after every received packet (no timer)
  regs[E1000_RADV] = 0; // interrupt after every packet (no timer)
  regs[E1000_IMS] = (1 << 7); // RXDW -- Receiver Descriptor Write Back
}

/*
* familiar with the interaction between driver and peripheral devices, memory mapped
* registers and DMA data transfer, and the core methods to implement the interaction
* with E1000 NIC: transmit and recv.
*/

/*
* when the operating system wants to send data, 
* it puts the data into the ring buffer array tx_ring 
* and then increments E1000_TDT, 
* and the NIC automatically sends out the data
*/
int
e1000_transmit(struct mbuf *m)
{
  //
  // Your code here.
  //
  // the mbuf contains an ethernet frame; program it into
  // the TX descriptor ring so that the e1000 sends it. Stash
  // a pointer so that it can be freed after sending.
  //
  acquire(&e1000_lock);
  // E1000_TDT is the Transmit Descriptor Tail register
  // this register indicates the index of the next avaliable buffer in the transmit ring.

  // 1. ask the E1000 for the TX ring index at which it's exepecting the next packet, by
  // reading the E1000_TDT control register.
  uint32 idx = regs[E1000_TDT];
  /* 
  * get the buffer's fd from the tx_ring, which contains lots of info about the buffer
  * 
  * your transmit code must place a pointer to the packet data in a descriptor
  * in the TX(transmit) ring. struct tx_desc describes the descriptor format.
  */
  struct tx_desc *desc = &tx_ring[idx];
  // E1000_TXD_STAT_DD is the initial value of each unit in tx_ring
  // which indicates that E1000 has finished transmitting
  // if the data in this buffer has not yet been transferred, we have used up the entire
  // ring buffer list, the buffer is low, and an error is returned
  
  // 2. check if the ring is overflow. If E1000_TXD_STAT_DD is not set in the descriptor
  // indexed by E1000_TDT, the E1000 hasn't finished the corresponding previous transmission 
  // request, so return error.
  if(!(desc->status & E1000_TXD_STAT_DD)){// indicates the buffer is avaliable for use
    release(&e1000_lock);
    return -1;
  }

  // if this idx still has mbufs that were previously sent
  // but not released, release it
  /*
  * if there was a previous mbuf associated with this 
  * descriptor (from a previous transmission), it frees that mbuf using mbuffree()
  * this ensures that memory resources are properly managed.
  */


  // 2. otherwise, use mbuffree() to free the last mbuf that was transmitted from
  // that descriptor
  if(tx_mbufs[idx]){
    /* 
    * you need to ensure that each mbuf is eventually freed, but only after the
    * E1000 has finished transmitting the packet(the E1000 sets the E1000_TXD_STAT_DD)
    * bit in the descriptor to indicate this
    */
    mbuffree(tx_mbufs[idx]);
    tx_mbufs[idx] = 0;
  }

  // fill the send descriptor with the memory address 
  // and length of the mbuf to be sent
  // it set the address field of the transmit descriptor to the 
  // memory address of the mbuf's data buffer

  // 3. fill in the descriptor. m->head points to the packet's content in memory,
  // and m->len is the packet length. 
  desc->addr = (uint64)m->head;
  desc->length = m->len;// set the length of the data to be transmiited
  // it sets command flags in the transmit descriptor, indicating that this
  // buffer contains the End of Packet the End of Packet(EOP) and that
  // the NIC should set the E1000_TXD_STAT_DD bit in the descriptor's status
  // after the transmission is complete.

  // 3. Set the necessary cmd flags
  desc->cmd = E1000_TXD_CMD_EOP | E1000_TXD_CMD_RS;// section 3.3 in the E1000 manual
  // it stores a pointer to the mbuf in the tx_mbufs array for later reference
  // when freeing the buffer

  // 3. and stash away a pointer to the mbuf for later freeing.
  tx_mbufs[idx] = m;

  // update the TDT register to indicate the next avaliable buffer in the transmit 
  // ring. The modulo operation ensures that the index wraps around 
  // when it reaches the end of the ring.

  // 4. finally, update the ring position by adding one to E1000_TDT modulo TX_RING_SIZE
  regs[E1000_TDT] = (regs[E1000_TDT] + 1) % TX_RING_SIZE;

  release(&e1000_lock);
  // 5. if e1000_transmit() added the mbuf successfully to the ring, return 0.
  // on failure(eg. there is no descriptor avaliable) to transmit the mbuf,
  // return -1 so that the caller knows to free the mbuf.
  return 0;
}

/*
* when the card receives the data, the card first used direct memory access
* to put the data into the rx_ring ring buffer array, and then initiates a 
* hardware interrupt to the CPU, which reads the data in the rx_ring directly
* after receiving the interrupt.
*/
static void
e1000_recv(void)
{
  //
  // Your code here.
  //
  // Check for packets that have arrived from the e1000
  // Create and deliver an mbuf for each packet (using net_rx()).
  //
  while(1){
    // 1. first ask the E1000 for the ring index at which the next waiting received packet(if any)
    // is located, by fetching the E1000_RDT control register and adding one modulo RX_RING_SIZE
    uint32 idx = (regs[E1000_RDT] + 1) % RX_RING_SIZE;
    struct rx_desc *desc = &rx_ring[idx];
    // 2. then check if a new packet is available by checking for the E1000_RXD_STAT_DD bit in
    // the status portion of the descriptor. if not, stop.
    if(!(desc->status & E1000_RXD_STAT_DD)) {
      return;
    }
    // 3. otherwise, update the mbuf's m->len to the length reported in the descriptor.
    
    rx_mbufs[idx]->len = desc->length;
    // 3. deliver the mbuf to the network stack using net_rx()
    net_rx(rx_mbufs[idx]);// called by e1000 driver's interrupt handler to deliver a packet to the networking stack
    // allocate a new mbuf and place it into the descritor
    // so that when the E1000 reaches that point in the RX ring again
    // it can find a fresh buffer into which to DMA a new packet

    // 4. then allocate a new mbuf using mbufalloc() to replace the one
    // just given to net_rx(). 
    rx_mbufs[idx] = mbufalloc(0);
    // 5. program its data pointer (m->head) into the descriptor
    desc->addr = (uint64)rx_mbufs[idx]->head;
    // 6. clear the descriptor's status bits to zero
    desc->status = 0;
    // 7. finally, update the E1000_RDT register to be the index of the last ring descriptor processed
    regs[E1000_RDT] = idx;
  }
}

void
e1000_intr(void)
{
  // tell the e1000 we've seen this interrupt;
  // without this the e1000 won't raise any
  // further interrupts.
  regs[E1000_ICR] = 0xffffffff;

  e1000_recv();
}
