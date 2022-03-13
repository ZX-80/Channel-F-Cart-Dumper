# Channel F Videocart Dumper

<img src="https://user-images.githubusercontent.com/44975876/155827105-ee87fd83-5722-4dc9-9ef0-5f88691cc5ed.png" align="right" width="30%" />

To my knowledge, all Channel F Videocarts have been dumped (except for a single Diagnostics Videocart mentioned in the service manual), and are available online. However, this project can still be useful for those that wish to:

- Dump their own collection of Videocarts for backup purposes
- Diagnose issues with any Videocarts
- Test and confirm a Videocart is in working condition without needing a functional Channel F

## F8 ROMC Commands

<p align="center">
    <img src="https://user-images.githubusercontent.com/44975876/156692174-16221c1e-fdfc-484e-ab8f-1bb6bf60f5e6.png">
</p>

The Fairchild Channel F uses the Fairchild F8 microprocessor system. It's pretty unique, as instead of using an address bus (which would require 16 lines) and a single program counter in the CPU, F8 systems use a control bus (which only requires 5 lines) and seperate program counters in each chip. This keeps the pin count, and therefore cost, low. Because of this, we can't just output to an address bus and read from a data bus. Instead we must output the appropriate commands to get the Videocarts internal program counter to the address we want, then use other commands to make the Videocart output that data to the data bus. Luckily there are specific commands provided to do this quickly and efficiently.

All ROMC commands are tested, except the following:

| **ROMC (43210)** | **HEX** | **Cycle** **length** | **Function**                                                 |
| --------------- | ------- | -------------------- | ------------------------------------------------------------ |
| 01111              | 0F      | L                    | The interrupting device with highest priority must place the low order byte of the interrupt vector on the data bus. All devices must copy the contents of PC0 into PC1. All devices must moce the contents of the data bus into the low order byte of PC0. |
| 10000              | 10      | L                    | Inhibit any modification to the interrupt priority logic.    |
| 10001           | 11      | L                 | The device whose memory space includes the contents of PC0 must place the contents of the addressed memory word onto the data bus. All devices must then move the contents of the data bus to the upper byte of DC0. |
| 10011              | 13      | L                    | The interrupting device with highest priority must move the high order half of the interrupt vector onto data bus. All devices must move the contents of the data bus into the high order byte of PC0. The interrupting device resets its interupt circuitry (so that it is no longer requesting CPU servicing and can respond to another interrupt). |
| 11010              | 1A      | L                    | During the prior cycle, an I/O port timer or interrupt control register was addressed; the device containing the addressed port must move the current contents of the data bus into the addressed port. |
| 11011              | 1B      | L                    | During the prior cycle, the data bus specified the address of an I/O port. The device containing the addressed I/O port must place the contents of the I/O port on the data bus. (Note that the contents of timer and interrupt control registers cannot be read back onto the data bus.) |

## Channel F Memory Map

The Channel F has a fairly simple memory layout. It's PC is 16-bits wide, for a total of 64 KB supported. The BIOS is present in the first 2K of memory, with the Videocart data beginning after it at 0x0800. So to read the Videocart, we need to issue commands that get the Videocarts internal PC to 0x0800, then read starting from that address. I chose to only read 4K of memory, as that is the maximum size of all official USA Videocarts. This can be trivially changed by modifying the provided code.

\* SRAM and LED are only on the SABA Videoplay 20 
<p align="center">
  <img src="https://user-images.githubusercontent.com/44975876/158039462-0ddce313-e5f4-45ea-be5a-597dad10f69e.png" width="90%" />
</p>

## Videocart to Arduino Pin Connections

<img src="https://user-images.githubusercontent.com/44975876/158038594-6b74264d-a01b-4688-a2d6-b7ea2ebc51cf.png" align="right" width="30%" />

The 3851 ICs used in Videocarts have an *Absolute Maximum Rating* of -0.3V to +15V, but recommend +12V, on VGG. So while it's possible to connect VGG to an Arduinos +5V pin, I would not recommend it. Instead you should use an external power source. My recomendation is a 12V DC power supply with its positive pin attached to the Videocarts +12V, and its GND pin connected to the Arduinos GND pin.

| Videocart pins                         | Arduino pins                   |
| -------------------------------------- | ------------------------------ |
| D0, D1, D2, D3, D4, D5, D6, D7         | 51, 49, 39, 35, 36, 32, 30, 28 |
| ROMC 0, ROMC 1, ROMC 2, ROMC 3, ROMC 4 | 45, 43, 41, 37, 40             |
| PHI                                    | 38                             |
| WRITE                                  | 34                             |
| GND                                    | GND                            |
| +5V                                    | +5V                            |
| +12V / VGG                             | N/A                            |

# Useful Links / Sources

[F8 - VES Wiki](https://channelf.se/veswiki/index.php?title=F8)

[ROMC - VES Wiki](https://channelf.se/veswiki/index.php?title=ROMC)

[F8 User's Guide (PDF)](http://hcvgm.org/Static/Books/F8%20User's%20Guide%20(1976)(Fairchild)(Document%2067095665)-1.pdf)
