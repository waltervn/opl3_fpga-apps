#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/i2c-dev.h>
#include <time.h>
#include <errno.h>

// Receives bank, reg, data in the following formats:
// SysEx:	0xf0 0x7d 0b0BRRRRRR 0b0RRDDDDD 0b0DDD0000 0xf7
// Note On:	0b10010DBR 0b0RRRRRRR 0b0DDDDDDD

#define MIDI_DEVICE "/dev/snd/midiC0D0"
#define SYSEX_SIZE 4

#define OPL3_FPGA_BASE 0x43c00000
#define OPL3_FPGA_SIZE 0x10000

#define IIC_SLAVE_ADDR 0b0011010

static unsigned char *opl3 = 0;
static int ssm2603 = 0;
#define VOL_MAX 6
#define VOL_MIN -73
#define VOL_OFFSET (127 - 6)

static void writeOplReg(int reg, int data) {
	opl3[reg] = data;
}

static void parseSysEx(unsigned char b1, unsigned char b2, unsigned char b3) {
	int reg = ((b1 & 0x7f) << 2) | (b2 >> 5);
	int data = ((b2 & 0x1f) << 3) | (b3 >> 4);

	writeOplReg(reg, data);
}

static void parseMsg(unsigned char b1, unsigned char b2, unsigned char b3) {
	int reg = ((b1 & 3) << 7) | b2;
	int data = ((b1 & 4) << 5) | b3;

	writeOplReg(reg, data);
}

static void setAudioReg(int file, unsigned char reg, unsigned short data) {
	unsigned char buf[2];

	buf[0] = (reg << 1) | ((data >> 8) & 1);
	buf[1] = data & 0xff;

	if (write(file, buf, 2) != 2) {
		perror("I2C failed");
		exit(1);
	}
}

static void setAudioVolume(int volume) {
	setAudioReg(ssm2603, 2, (1 << 8) | (volume + VOL_OFFSET));
	printf("Current volume: %i dB\n", volume);
}

static void initAudio() {
	ssm2603 = open("/dev/i2c-0", O_RDWR);

	if (ssm2603 < 0) {
		perror("Couldn't open /dev/i2c-0");
		exit(1);
	}

	if (ioctl(ssm2603, I2C_SLAVE, IIC_SLAVE_ADDR) < 0) {
		perror("Failed ioctl");
		exit(1);
	}

	// setAudioReg(file, 15, 0b000000000); //Perform Reset
	usleep(75000);
	setAudioReg(ssm2603, 6, 0b000110000); //Power up
	setAudioReg(ssm2603, 0, 0b000010111);
	setAudioReg(ssm2603, 1, 0b000010111);
	setAudioVolume(0);
	setAudioReg(ssm2603, 4, 0b000010000);
	setAudioReg(ssm2603, 5, 0b000000000);
	setAudioReg(ssm2603, 7, 0b000001010);
	setAudioReg(ssm2603, 8, 0b000000000); //Changed so no CLKDIV2
	usleep(75000);
	setAudioReg(ssm2603, 9, 0b000000001);
	setAudioReg(ssm2603, 6, 0b000100000);
}

inline void delay() {
	// FIXME some kind of sub-microsecond delay here?
	struct timespec t;
	t.tv_sec = 0;
	t.tv_nsec = 0;
	nanosleep(&t, NULL);
}

int main(int argc, char *argv[])
{
	initAudio();

	int mem_fd;
	int volume = 0;

	if ((mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("Error opening /dev/mem");
		return 1;
	}

	opl3 = mmap(NULL, OPL3_FPGA_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, OPL3_FPGA_BASE);

	if (!opl3) {
		perror("mmap error");
		close(mem_fd);
		return 1;
	}

	printf("Start MIDI slave mode\n");
	unsigned char buf[256];
	unsigned char sysEx[SYSEX_SIZE];
	int sysExSize = 0;
	unsigned char msg[4];
	int msgSize = 0;
	int fd = open(MIDI_DEVICE, O_RDONLY, 0);

	if (fd == -1) {
		perror("Error opening " MIDI_DEVICE);
		return 1;
	}

	int readingSysEx = 0;

	int run = 1;
	while (run) {
		int result;
		fd_set fdSet;
		do {
			FD_ZERO(&fdSet);
			FD_SET(fd, &fdSet);
			FD_SET(STDIN_FILENO, &fdSet);

			result = select(fd + 1, &fdSet, NULL, NULL, NULL);
		} while (result == -1 && errno == EINTR);

		if (result < 0) {
			perror("Select error");
			exit(1);
		}

		if (FD_ISSET(fd, &fdSet)) {
			int count = read(fd, buf, sizeof(buf));

			if (count <= 0) {
				fprintf(stderr, "MIDI socket read failed, exiting");
				exit(1);
			}

			int i;
			for (i = 0; i < count; ++i) {
				unsigned char c = buf[i];
				if (!readingSysEx) {
					if (c == 0xf0) {
						// Start reading SysEx
						readingSysEx = 1;
					} else {
						msg[msgSize++] = c;
						if (msgSize == 3) {
							if ((msg[0] & 0xf8) == 0x90) {
								parseMsg(msg[0], msg[1], msg[2]);
								delay();
							} else
								fprintf(stderr, "Ignoring unrecognized MIDI command: %02x %02x %02x", msg[0], msg[1], msg[2]);

							msgSize = 0;
						}
					}
				} else {
					if (c == 0xf7) {
						// Terminator received
						if (sysExSize == SYSEX_SIZE) {
							parseSysEx(sysEx[1], sysEx[2], sysEx[3]);
							delay();
						} else
							fprintf(stderr, "Invalid SysEx of size %i received\n", sysExSize);

						readingSysEx = 0;
						sysExSize = 0;
					} else if (sysExSize == SYSEX_SIZE) {
						fprintf(stderr, "SysEx overflow, ignoring data %02x\n", c);
						fprintf(stderr, "%02x %02x %02x %02x\n", sysEx[0], sysEx[1], sysEx[2], sysEx[3]);
						readingSysEx = 0;
						sysExSize = 0;
					} else {
						sysEx[sysExSize++] = c;
					}
				}
			}
		}

		if (FD_ISSET(0, &fdSet)) {
			int c = fgetc(stdin);
			switch(c) {
			case '-':
				if (volume > VOL_MIN) {
					--volume;
					setAudioVolume(volume);
				}
				break;
			case '+':
				if (volume < VOL_MAX) {
					++volume;
					setAudioVolume(volume);
				}
				break;
			case 'q':
				run = 0;
				break;
			}
		}
	}

	close(fd);
	close(ssm2603);
	munmap(opl3, OPL3_FPGA_SIZE);
	close(mem_fd);
	return 0;
}
