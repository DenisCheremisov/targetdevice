#ifndef _TARGETDEVICE_H_DEFINED_
#define _TARGETDEVICE_H_DEFINED_

int port_open(char *portname);
int port_talk(int fd, char *command, char *response);

#endif
