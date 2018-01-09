# MIDI daemon for OPL3-FPGA using PetaLinux

This allows Greg Taylor's [OPL3-FPGA](https://github.com/gtaylormb/opl3_fpga) to be controlled from a PC using USB-MIDI. The main purpose is to allow the use of OPL3-FPGA with modified versions of [ScummVM](http://www.scummvm.org) and [DOSBox](http://www.dosbox.com) through a USB connection.

## News
2018-1-8 
* This project has been updated for Vivado/PetaLinux 2017.4.
* Pre-built SD card image files are in the "images" folder. Feel free to copy these to a MicroSD card and skip to the "Running" section of the README.

## Building

1. Install Vivado 2017.4 and PetaLinux 2017.4.

2. Set up the environment:

        source /path/to/vivado/settings64.sh
        source /path/to/petalinux/settings.sh

3. Download Greg Taylor's [OPL3_FPGA BSP](https://github.com/gtaylormb/fpga_utility/blob/master/petalinux_bsp/ZYBO_OPL3_FPGA_2017_4.bsp)

4. Create a new PetaLinux project:

        petalinux-create -t project -s ZYBO_OPL3_FPGA_2017_4.bsp

5. Clone the Vivado/PetaLinux-2017.4 branch of this git repository and copy the "midi" and "opl3d" folders into:

        <project-folder>/project-spec/meta-user/recipes-apps

6. Edit <project-folder>/project-spec/meta-user/recipes-bsp/device-tree/files/system-user.dtsi and add the following:

        /{
            usb_phy0: usb_phy@0 {
            compatible = "usb-nop-xceiv";
            #phy-cells = <0>;
            };
        };
        
        &usb0 {
            dr_mode = "otg";
            usb-phy = <&usb_phy0>;
        };

7. Configure the kernel:

        petalinux-config -c kernel

  Enable the following options:
  - Device Drivers > USB support > USB Gadget Support -> MIDI function (*)

  Then exit and save the configuration

8. Configure the apps:

        petalinux-config -c rootfs

  Enable the following options:
  - Apps > midi (*)
  - Apps > opl3d (*)

  Then exit and save the configuration

9. Build Linux:

        petalinux-build

10. Use petalinux-package to generate BOOT.BIN:

        petalinux-package --boot --fsbl images/linux/zynq_fsbl.elf --fpga images/linux/opl3.bit --u-boot

## Running

1. Copy BOOT.BIN and image.ub (from images/linux) onto a MicroSD card and insert it into the ZYBO

2. Set JP5 to SD to enable booting from the MicroSD card

3. Remove JP1 to enable OTG mode

4. Use a Micro-USB cable to connect J9 (on the *bottom* side of the ZYBO!) to your PC.

5. Power on the ZYBO

## Use with ScummVM

1. Clone https://github.com/waltervn/scummvm.git and build the 'opl3-fpga' branch.

2. In the ScummVM launcher, go to Options > Audio and select "OPL3 FPGA" in the Adlib emulator list box. Click OK to save. Any games set up for AdLib will now use OPL3-FPGA.

## Use with DOSBox

1. Clone https://github.com/waltervn/dosbox.git and build the 'opl3-fpga' branch.

2. Edit dosbox.conf and make the following changes:

  - Under [sblaster] set oplmode to 'opl3fpga'.

  - Under [midi] set midiconfig to point to the OPL3-FPGA midi device.
  
  - Also under [midi] set oplrate=49716. (Walter, this is the rate the OPL3-FPGA is running at, but does this do anything?)

    For Linux, try the following:
    Run 'aconnect -l' and look for "MIDI function". Use the client number you see there to set 'midiconfig'. E.g. if your client number is 28, set midiconfig to "28:0".

    Other operating systems should work as well, as long as DOSBox has MIDI support for that platform, and you can point DOSBox to the OPL3-FPGA.
