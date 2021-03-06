# dpdk_qat
A clone of the dpdk_qat directory from the DPDK tree with modification to support hardware assisted compression operations

This is a clone of the dpdk_qat directory of the DPDK tree rev 16.07.

In the DPDK source tree, this directory is the sample code for using Intel 
QuickAssist technology for hardware assisted cryptographic operations.

Modification is made in this directory, to have a prove of concept to use the
Intel QuickAssist hardware to perform compression and decompression operations.

As a dependency, the Intel QuickAssist source code is needed as the functions
in crypto.c and comp.c will reference /call API functions in the Intel
QuickAssist source code.  Use the installer.sh in the Intel QuickAssist source
tree to build the binaries as this sample code in dpdk_qat needs to link some
libraries from the QuickAssist tree.  The installer.sh script will guide the
installation process and is well written.  For a more detailed description on
building and/or installing the software please refer to "Intel Communications
Chipset 89xx Series Software for Linux".  To build and/or install this
software, we need a QAT card on the Linux system. (Something I do not
currently have).

Also before few variables needs to be set.  Both the DPDK source tree
.../doc/guides and/or "http://dpdk.org/doc/guides/linux_gsg/index.html" has
good and precise instructions.

export RTE_SDK=$HOME/DPDK
export RTE_TARGET=x86_64-native-linuxapp-gcc

The installer.sh script for Intel QuickAssist software should set the ICP_ROOT
correctly but since at this time I do not have the hardware, I have to set
this manually:
export ICP_ROOT=$HOME/qatmux.1.2.6.0-60/QAT1.6

I have modified the main.c to detect a new runtime option run_compress and
this will direct the code to run the functions in the newly created comp.c

Original syntax is:

    dpdk_qat [EAL options] – -p PORTMASK [–no-promisc] [–config ‘(port,queue,lcore)[,(port,queue,lcore)]’]

where,

    -p PORTMASK: Hexadecimal bitmask of ports to configure
    –no-promisc: Disables promiscuous mode for all ports, so that only packets
with the Ethernet MAC destination address set to the Ethernet address of the
port are accepted. By default promiscuous mode is enabled so that packets are
accepted regardless of the packet’s Ethernet MAC destination address.

now 
    dpdk_qat [EAL options] – -p PORTMASK [–run-compress] [–no-promisc] [–config ‘(port,queue,lcore)[,(port,queue,lcore)]’]

This proof of concept in comp.c is work in progress and since I do not have
the necessary hardware (QAT card), this code is NOT tested.  The code is to
read in a file called 'uncompressedFile' and call the Intel QuickAssist API 
cpaDcCompressData() for the compression operation and then write the
compressed contect into a new file called 'compressedFile'.  Later we can add in the 
decompression operations and write the decompressed content into a file called 
'newUncompressedFile'. 


Additional resources:
http://dpdk.org/doc/guides/sample_app_ug/intel_quickassist.html
