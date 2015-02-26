#!/usr/bin/env python
# encoding: utf-8

import socket, ssl


bindsocket = socket.socket()
bindsocket.bind(('', 10023))
bindsocket.listen(5)


def readout(connstream):
    data = connstream.read()
    return data


def deal_with_client(connstream):
    data = readout(connstream)
    print data
    if data.rstrip() != 'ready':
        response = '0:error-intro'
    else:
        response = '0:is-connected'
    connstream.write(response)
    return


newsocket, fromaddr = bindsocket.accept()
connstream = ssl.wrap_socket(newsocket,
                             server_side=True,
                             certfile="data/server.crt",
                             keyfile="data/server.key")
while True:
    try:
        deal_with_client(connstream)
    except Exception as e:
        break
