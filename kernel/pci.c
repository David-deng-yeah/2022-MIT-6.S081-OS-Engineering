//
// simple PCI-Express initialization, only
// works for qemu and its e1000 card.
//

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

/*
* code that searches for an E1000 card on the PCI bus when xv6 boots
* PCI bus refers to the Peripheral component interconnect bus, which
* is hardware interface standard used for connecting various hardware
* components, such as network cards, graphics cards, and other peripheral
* device, to a computer's motherboard.
* 
* pci_init are responsible for scanning and searching for an E1000 card on the PCI bus
* when xv6 boots
*/
void
pci_init()
{
  // we'll place the e1000 registers at this address.
  // vm.c maps this range.
  // the base address where the E1000 network card's registers
  // will be mapped.
  uint64 e1000_regs = 0x40000000L;

  // qemu -machine virt puts PCIe config space here.
  // vm.c maps this range.
  // base address for PCIe configuration space, which is used for
  // configuring and identifying PCI devices.
  uint32  *ecam = (uint32 *) 0x30000000L;
  
  // look at each possible PCI device on bus 0.
  // E1000 Network Card Detection
  for(int dev = 0; dev < 32; dev++){
    int bus = 0;
    int func = 0;
    int offset = 0;
    uint32 off = (bus << 16) | (dev << 11) | (func << 8) | (offset);
    volatile uint32 *base = ecam + off;
    uint32 id = base[0];
    
    // 100e:8086 is an e1000
    // check whether the current PCI device is an E1000 card.
    if(id == 0x100e8086){
      // command and status register.
      // bit 0 : I/O access enable
      // bit 1 : memory access enable
      // bit 2 : enable mastering
      // sets the status register of the E1000 card to enable
      // I/O access, memory access and mastering
      base[1] = 7;// 7(base 10) -> 111(base 2)
      __sync_synchronize();
      // iterates over the Base Address Registers (BARs) associated with
      // the E1000 card
      for(int i = 0; i < 6; i++){
        uint32 old = base[4+i];

        // writing all 1's to the BAR causes it to be
        // replaced with its size.
        base[4+i] = 0xffffffff;
        __sync_synchronize();

        base[4+i] = old;
      }

      // tell the e1000 to reveal its registers at
      // physical address 0x40000000.
      base[4+0] = e1000_regs;
      // initialize the E1000 network card.
      e1000_init((uint32*)e1000_regs);
    }
  }
}
