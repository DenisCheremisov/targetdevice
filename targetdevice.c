#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <memory.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "confparser.h"


int set_interface_attribs (int fd, int speed, int parity) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if(tcgetattr (fd, &tty) != 0) {
        perror("get attribute");
        return -1;
    }

    cfsetospeed (&tty, speed);
    cfsetispeed (&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if(tcsetattr (fd, TCSANOW, &tty) != 0) {
        perror("set attribute");
        return -1;
    }
    return 0;
}


int set_blocking (int fd, int should_block) {
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (fd, &tty) != 0) {
        perror("get blocking attribute");
        return -1;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if(tcsetattr (fd, TCSANOW, &tty) != 0) {
        perror("set blocking attribute");
        return -1;
    }
    return 0;
}


int port_open(char *portname) {
    int fd;
    if(portname == NULL) {
        portname = "/dev/usb/tts/0";
    }
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if(fd <= 0) {
        fprintf(stderr, "cannot open serial port %s", portname);
        perror(" ");
        return 0;
    }
    if(set_interface_attribs(fd, B115200, 0) < 0) { // set speed to 115,200 bps, 8n1 (no parity)
        return 0;
    }
    if(set_blocking(fd, 1) < 0) {// set no blocking
        return 0;
    }
    return fd;
}


int port_talk(int fd, char *command, char *response) {
    int i, n;
    // Check if the command has proper format ($.....\r\n)
    // Effective command length is limited to 100 symbols (not including \r\n)
    if(command[0] != '$') {
        return -1;
    }
    for(i = 1; i < 100; i++) {
        if(command[i] < 32) {
            break;
        }
    }
    if(command[i] != '\r' || command[i+1] != '\n') {
        fputs("serial port command must ends with \\r\\n\n", stderr);
        fflush(stderr);
        return -1;
    }

    write(fd, command, i+2); // Write command
    n = read(fd, response, 255);
    response[n] = '\0';
    return 0;
}


char* command_format(char *command) {
    char *result, *pos;

    pos = strchr(command, '\r');
    if(pos == NULL) {
        return NULL;
    }
    result = (char*)malloc(sizeof(char)*((pos - command) + 1));
    if(result == NULL) {
        return NULL;
    }
    strncpy(result, command, pos - command);
    result[pos - command] = 0;
    return result;
}
