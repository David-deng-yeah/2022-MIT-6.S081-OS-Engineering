# lab
1. **Understand the Basics**:
   - Review the relevant chapter on interrupts and device drivers in the xv6 book.
   - Familiarize yourself with the E1000 controller by reading its Software Developer's Manual, especially the sections mentioned in your instructions (Chapters 2, 3, 13, and 14).

2. **Code Structure**:
   - The lab provides some starting code in the `kernel/e1000.c` file, including initialization functions.
   - You need to complete the `e1000_transmit()` and `e1000_recv()` functions in `kernel/e1000.c` to enable packet transmission and reception.

3. **e1000_transmit() Function**:
   - Start by adding print statements to `e1000_transmit()` to track its execution and interactions with other components.
   - Retrieve the TX ring index from the `E1000_TDT` control register to know where the NIC expects the next packet.
   - Check for ring overflow by inspecting the status of the descriptor at the current `E1000_TDT` index. If it's not marked as done (`E1000_TXD_STAT_DD`), return an error.
   - If the descriptor is marked as done, use `mbuffree()` to free the last mbuf transmitted from that descriptor.
   - Populate the descriptor with the necessary information (packet data pointer, length, and command flags).
   - Update the ring position by incrementing `E1000_TDT` modulo `TX_RING_SIZE`.
   - Return 0 on success and -1 on failure.

4. **e1000_recv() Function**:
   - As with `e1000_transmit()`, add print statements to `e1000_recv()` to aid debugging.
   - Get the ring index for the next waiting received packet using the `E1000_RDT` control register and adding one modulo `RX_RING_SIZE`.
   - Check if a new packet is available by examining the status of the descriptor. If the `E1000_RXD_STAT_DD` bit is not set, stop.
   - If a new packet is available, update the mbuf's length and deliver it to the network stack using `net_rx()`.
   - Allocate a new mbuf using `mbufalloc()` to replace the one given to `net_rx()`. Program its data pointer and clear the descriptor's status bits.
   - Update the `E1000_RDT` register to the index of the last processed ring descriptor.

5. **Handling Ring Overflow**:
   - Make sure your code can handle scenarios where the total number of packets received exceeds the ring size (16 in your instructions).
   
6. **Concurrency and Locks**:
   - Implement proper locks and synchronization mechanisms to handle multiple processes or kernel threads accessing the E1000 simultaneously.

7. **Testing**:
   - To test your driver, run `make server` in one window and `make qemu` in another. Then run `nettests` in xv6.
   - You should also inspect the recorded packets using `tcpdump` as mentioned in your instructions.

8. **Gradual Debugging**:
   - Start by implementing the basic functionality and gradually add complexity while thoroughly testing each step.

9. **Reference Existing Code**:
   - Refer to the existing code in `e1000_init()` for initializing the RX ring with mbufs. You can reuse or adapt this code.

10. **Complete All Required Functions**:
   - Ensure that you complete both `e1000_transmit()` and `e1000_recv()` functions as they are essential for packet transmission and reception.

Remember that developing a network driver is a challenging task, and debugging will likely be a significant part of your work. Be patient and systematic in your approach, and make good use of print statements and debugging tools to track the flow of your code.

# lecture Networing

## LAN (Local Area Network)
a LAN need to be able to allow all the hosts in it to receive packets from each other

from the perspective of network protocols, LAN communication is dertermined by the Ethernet Po=rotocol. And long distance network communication above the LAN is determined by the internet Protocol.

Each Ethernet packet starts with a Header, which contains 3 pieces of data, followed by the payload data. The three pieces of data in the Header are:
* the destination Ethernet address
* the source Ethernet address
* the type of packets

Each Ethernet address is a 48-bit number that uniquely identifies a network-card. The type of packets tells the host on the receiving and what to do with the packet.

the Ethernet packets is comprised with 48bit+48bit Ethernet address, the 16bit type, and the payload of any length.

composition of packets: 
(Preamble + SFD) + packet + (FCS) + payload

the flags at the beginning and end of the packets will not be seen by the system kernel, the rest will be sent from the NIC to the system kernel.

file kernel/net.h contains definitions of packet headers for lots of network protocols.

Although Ethernet addresses (EIC, 48 bits) are unique, out of the LAN they are not helpful in locating the destination host.
if the destination host of a network communication is on the same LAN, then the destination host listens for packets sent to its own address.
but if the network communication takes place between hosts in two countries, you need to use a different addressing method, and that's where IP addresses come in.

you can use tcpdump to inspect Ethernet packet, and the output contains information like:
* 48 bits Ethernet address
* 16bit types of packet, like ARP

## ARP
we need to find the 48bit Ethernet address of the corresponding host from the 32bit IP address. This is done through a dynamic resolution protocol, the Address Resolution Protocol, ARP.

when a packet arrives at a router and need to forwarded to another host on the same Ethernet, it will broadcasts an ARP packet across the LAN, and if the corresponding host exits and is powered on, it sends an ARP response packets to the sender.

```c
// an ARP packet (comes after an Ethernet header).
struct arp {
    uint16 hrd;     // format of hardware address
    uint16 pro;     // format of protocol address
    uint8  hln;     // length of hardware address
    uint8  pln;     // length of protocol address
    uint16 op;      // operation

    char    sha[ETHADDR_LEN];   // sender hardware address
    uint32  sip;                // sender IP address
    char    tha[ETHADDR_LEN];   //target hardware address
    uint32  tip;                //target Ip address
}__attribute__((packed));
#define ARP_HRD_ETHER 1 // Ethernet
enum {
    ARP_OP_REQUEST = 1,// requests hw addr given protocol addr
    ARP OP REPLY =2， // replies a hw addr given protocol addr
}
```

when an OS receives a packet, it parses the first header and knows that it is Ethernet, and after some legitimacy checking, the Ethernet header is stripped and the OS parses the next header. The Ethernet header contains a type field that indicates how to parse the next header. Similarly, the ip header contains a protocol field that indicates how to parse the next header.

the software will parses each header, does a checksum, strips the header, and gets the next header, repeating this process until it gets to the final data. This is a nested packet header.

## internet
```c
// an IP packet header (comes after an Ethernet header).
struct ip {
  uint8  ip_vhl; // version << 4 | header length >> 2
  uint8  ip_tos; // type of service
  uint16 ip_len; // total length
  uint16 ip_id;  // identification
  uint16 ip_off; // fragment offset field
  uint8  ip_ttl; // time to live
  uint8  ip_p;   // protocol
  uint16 ip_sum; // checksum
  uint32 ip_src, ip_dst;
};
```

the IP header is retained all the way through a packet being sent to a network on the other side of the world, while the Ethernet header is stripped after it leaves the local Ethernet.
the ip header has global significance, whereas the Ethernet header has significance only on a single LAN.

the key information is in three parts: the destination IP address, the source IP address and the protocol. The high bit in the address is the network number, which will help the router with routing. The protocol field in the IP header tells the destination host how to handle the IP payload.

if you send a packet to a specific IP address, your host will first check the destination IP address of the packet to determine if the destination host is on the same LAN as your host. If it is, your host will directly use ARP to translate the IP address into an Ethernet. 
A more common scenario is when we send a packet to a host on the Internet. In this case, your host will send a packet to a router on the LAN, which will check the destination IP address of the packet and select the next router according to the routing table to forward the packet to.




# DMA(Direct Memory Access)
The E1000 network interface card (NIC) uses a technique known as Direct Memory Access (DMA) to read and write packets directly to and from system memory (RAM). DMA is a mechanism that allows hardware devices, such as network cards, to access the main memory of a computer system without involving the CPU in every data transfer. Here's how it works:

1. **Initialization**: During the initialization phase of the NIC (as done in the `e1000_init()` function), the NIC is configured to use DMA for packet transfers. This involves setting up DMA descriptors and control registers on the NIC to specify how data should be transferred between RAM and the NIC's internal buffers.

2. **DMA Descriptors**: The NIC uses a set of DMA descriptors or buffers to manage data transfers. These descriptors contain information about the location in RAM where packets should be read from or written to, the size of the data to transfer, and other control information.

3. **Packet Reception**:
   - When the NIC receives a network packet, it reads the packet data directly from the network medium (e.g., Ethernet cable) into its internal buffers.
   - The NIC then uses a DMA descriptor to specify the location in RAM where the received packet should be written.
   - The NIC's DMA controller copies the packet data from its internal buffers to the specified location in RAM without involving the CPU.

4. **Packet Transmission**:
   - When the CPU instructs the NIC to transmit a network packet, it provides the NIC with a DMA descriptor that points to the location in RAM where the packet data is stored.
   - The NIC's DMA controller reads the packet data from RAM and sends it out onto the network medium.

5. **Minimal CPU Involvement**: Importantly, the CPU's involvement in these data transfers is minimized. Once the NIC is properly configured and provided with the necessary descriptors, the CPU can continue executing other tasks without having to copy packet data to or from the NIC manually.

6. **Efficiency and Performance**: DMA significantly improves the efficiency and performance of network communication because it reduces the overhead of CPU involvement in data transfers. This is especially important in high-speed network environments.

In summary, the E1000 NIC uses DMA to directly read and write network packets to and from RAM, which improves network performance and reduces CPU overhead. DMA is a crucial technology for efficient data transfer between hardware devices and system memory in computer systems.


