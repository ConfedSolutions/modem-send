/*
 ============================================================================
 Name        : modem-send.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <termios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/types.h>
#include <unistd.h>
#include <err.h>
#include <signal.h>
#include <ctype.h>
#include <sys/sysmacros.h>

static int baudrate_to_line_speed(int baud);

int main(int argc, char **argv)
{
	// parse the command line options
	int do_verbose = 0;
	int timeout_ms = 1000;
	int baudrate = 115200;
	int line_speed = B115200;
	const char *dev_name = "/dev/ttymdmAT1";

	int opt = 0;
	while ((opt = getopt(argc, argv, "b:d:t:v")) != -1)
	{
		switch (opt)
		{
		case 'b':
		{
			// parse the baudrate value
			char *end;
			baudrate = strtol(optarg, &end, 10);
			char *expected_end = optarg + strlen(optarg);
			if (expected_end != end)
				goto print_usage;
		} break;

		case 'd':
		{
			// get the device filename
			dev_name = optarg;
		} break;

		case 't':
		{
			// parse the timeout value
			char *end;
			timeout_ms = strtol(optarg, &end, 10);
			char *expected_end = optarg + strlen(optarg);
			if (expected_end != end)
				goto print_usage;

			// check if the timeout is within bounds
			if (timeout_ms > 25)
				errx(-EXIT_FAILURE, "timeout cannot exceed 25 seconds");

			// convert to milliseconds
			timeout_ms *= 1000;
		} break;

		case 'v':
		{
			do_verbose = 1;
		} break;

		default:
			goto print_usage;
		}
	}

	// convert to line speed
	line_speed = baudrate_to_line_speed(baudrate);

	// check if the device exists
	struct stat stat_buffer;
	if (stat(dev_name, &stat_buffer) != 0)
		errx(-EXIT_FAILURE, "device not found: %s", dev_name);

	// check if a command is specified
	int has_command = (argc > optind);
	if (! has_command)
		errx(-EXIT_FAILURE, "no modem command given");

	// open the serial port
	int ser_fd = open(dev_name, O_RDWR | O_NOCTTY);
	if (ser_fd == -1)
		errx(-EXIT_FAILURE, "cannot open device: %s", dev_name);

	// get the current attributes of the serial port
	struct termios tio;
	if (tcgetattr(ser_fd, &tio) == -1)
		errx(-EXIT_FAILURE, "cannot get line attributes");

	// set the new attributes
	tio.c_iflag = 0;
	tio.c_oflag = 0;
	tio.c_cflag = CS8 | CREAD | CLOCAL | CRTSCTS;
	tio.c_lflag = 0;
	tio.c_cc[VMIN] = 0;
	tio.c_cc[VTIME] = (timeout_ms / 100);

	// set the speed of the serial line
	if (cfsetospeed(&tio, line_speed) < 0 || cfsetispeed(&tio, line_speed) < 0)
		errx(-EXIT_FAILURE, "cannot set line speed");

	// set the attributes
	if (tcsetattr(ser_fd, TCSANOW, &tio) == -1)
		errx(-EXIT_FAILURE, "cannot set line attributes");

	// send the AT command to the modem
	int cmd_len = strlen(argv[optind]);
	if (cmd_len > 4094)
		errx(-EXIT_FAILURE, "command to long, max length 4094");
	char command[4096] = { 0 };
	memcpy(command, argv[optind], cmd_len);
	command[cmd_len++] = '\r';
	command[cmd_len] = 0;
	if (do_verbose)
		printf(">> %s\n", command);

	// send the command to the modem
	int wrote = write(ser_fd, command, cmd_len);
	if (wrote != cmd_len)
		errx(-EXIT_FAILURE, "couldn't write command to the modem");

	// read the response of the modem
	char in_buffer[4096] = { 0 };
	int size = read(ser_fd, in_buffer, sizeof(in_buffer));
	if (size < 0)
	{
		errx(-EXIT_FAILURE, "failed to read from modem");
	}
	else if (size == 0)
	{
		errx(-EXIT_FAILURE, "no response from modem");
	}
	else
	{
		if (do_verbose)
			printf("<< %s", in_buffer);
		else
			printf("%s", in_buffer);
	}

	return (strstr(in_buffer, "\r\nOK\r") != NULL)
			? EXIT_SUCCESS
			: -EXIT_FAILURE;

print_usage:
	printf("usage: %s [-t timeout_seconds] [-b baudrate] [-d /dev/ttymdmAT1] command\r\n", argv[0]);

	return -EXIT_FAILURE;
}

int baudrate_to_line_speed(int baud)
{
	switch(baud)
	{
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		case 230400: return B230400;
		case 460800: return B460800;
		case 921600: return B921600;
		default:
			errx(-EXIT_FAILURE, "invalid baudrate: %d", baud);
	}
}
