#!/usr/bin/env python
# encoding: utf-8

import socket, ssl


bindsocket = socket.socket()
bindsocket.bind(('', 10023))
bindsocket.listen(5)
bindsocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)


def deal_with_client(connstream):
    data = connstream.read()
    print data
    if data.rstrip().split(':')[0] != 'ready':
        response = '0:error-intro'
    else:
        response = '\n'.join([
            '0:is-connected',
            '1:adc-get:channel=2'
            ])
    connstream.write(response)
    print connstream.read()
    return


while True:
    newsocket, fromaddr = bindsocket.accept()
    connstream = ssl.wrap_socket(newsocket,
                                server_side=True,
                                certfile="data/server.crt",
                                keyfile="data/server.key")
    try:
        deal_with_client(connstream)
    except Exception as e:
        pass
