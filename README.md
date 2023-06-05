# Picofly
Information and firmware related to the rp2040-zero based chip for the switch.
all credits goto Rehius, flynnsmt4, Vittorio and anyone else who helped for the released firmwares


# Troubleshooting
As of firmware 2.70 and beyond, the debug led color and codes have chanaged.

- now it's only 3 colours: blue (glitching), white (flashing), yellow (error code). 
This was made to make possible pi pico debugging + get rid of RGB/GRB issues

### Error codes list (= is long pulse, * is short pulse):

- = USB flashing done

- ** RST is not connected
- *= CMD is not connected
- =* D0 is not connected
- == CLK is not connected
 
- =** eMMC init failure during glitch process
- =*= CPU never reach BCT check, should not happen
- ==* CPU always reach BCT check (no glitch reaction, check mosfet)
- === Glitch attempt limit reached, cannot glitch

- =*** eMMC init failure
- =**= eMMC write failure - comparison failed
- =*=* eMMC write failure - write failed
- =*== eMMC test failure - read failed
- ==** eMMC read failed during firmware update
- ==*= BCT copy failed - write failure
- ===* BCT copy failed - comparison failure
- ==== BCT copy failed - read failure

The second major feature is CPU downvoltage. This might be useful when your MOSFET (or the wire) is not strong enough for the glitch. (do you remember the case where you press "RESET" on the rp2040 when joycon logo appears to make it working? that's it, system lowers CPU voltage)
Therefore you can solder two additional wires to the chip so it could lower the CPU voltage making the glitch easier. This is optional! only if you really need.

- Waveshare rp2040: SDA=12, SCL=13
- Pi Pico: SDA = 19, SCL = 20
- XIAO 2040: SDA=3, SCL=4
- ItayBitsy 2040: SDA = 18, SCL = 19
