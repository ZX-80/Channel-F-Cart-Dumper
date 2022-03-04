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

The following commands were used

| **ROMC (43210)** | **HEX** | **Cycle** **length** | **Function**                                                 |
| --------------- | ------- | -------------------- | ------------------------------------------------------------ |
| 00000           | 00      | S, L                 | Instruction Fetch. The device whose address space includes the contents of the PCO register must place on the data bus the op code addressed by PC0; then all devices increment the contents of PC0. |
| 01000           | 08      | L                  | All devices copy the contents of PC0 into PC1. The CPU outputs zero on the data bus in this ROMC state. Load the data bus into both halves of PC0, thus clearing the register. |
| 10100           | 14      | L                    | All devices move the contents of the data bus into the high low order byte of PC0. |
| 10111           | 17      | L                    | All devices move the contents of the data bus into the low order byte of PC0. |

## Channel F Memory Map

The Channel F has a fairly simple memory layout. It's PC is 16-bits wide, for a total of 64 KB supported. The BIOS is present in the first 2K of memory, with the Videocart data beginning after it at 0x0800. So to read the Videocart, we need to issue commands that get the Videocarts internal PC to 0x0800, then read starting from that address. I chose to only read 4K of memory, as that is the maximum size of all official USA Videocarts. This can be trivially changed by modifying the provided code.

| Address (hex) | Description   | Notes                                    |
| ------- | ------------- | ---------------------------------------- |
| 00xx    | BIOS          |                                          |
| 08xx    | Videocart ROM | Only used by 2K - 6K Videocarts          |
| 10xx    | Videocart ROM | Only used by 4K - 6K Videocarts          |
| 18xx    | Videocart ROM | Only used by 6K Videocarts               |
| 20xx    | Unused        |                                          |
| 28xx    | 2K of RAM     | Only on SABA Videoplay 20 |
| 30xx    | Unused        |                                          |
| 38xx    | LED           | Only on SABA Videoplay 20                |
| ...     | Unused        |                                          |
| F8xx    | Unused        |                                          |


# Useful Links / Sources

[F8 - VES Wiki](https://channelf.se/veswiki/index.php?title=F8)

[ROMC - VES Wiki](https://channelf.se/veswiki/index.php?title=ROMC)

[F8 User's Guide (PDF)](http://hcvgm.org/Static/Books/F8%20User's%20Guide%20(1976)(Fairchild)(Document%2067095665)-1.pdf)
