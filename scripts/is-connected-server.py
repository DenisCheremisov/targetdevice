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
        response = [
            '0:is-connected',
            '1:adc-get:channel=2',
            '2:adc-get:channel=1',
            '3:read-all',
            '4:read-line:lineno=1',
            '5:read-line:lineno=2',
            '6:write-line:lineno=2:value=0',
            '7:relay-set:relayno=2:value=0',
            '8:relay-set:relayno=5:value=5',
            '9:relay-set:relayno=1:value=5',
            '10:io-get-all',
            '11:io-get:lineno=5',
            '12:io-set:lineno=5:value=1',
            '13:io-get-all',
            '14:read-all',
            '15:read-line:lineno=5',
            '16:write-line:lineno=5:value=1',
            '17:read-all',
            '18:io-set:lineno=5:value=0',
            '19:read-all',
            '20:read-line:lineno=5',
            '21:afr-set:value=2000',
            '22:afr-set:value=200'
            ]
    connstream.write('\n'.join(response))
    for ___ in zip(response, connstream.read().split('\n')):
        print '{:32} -> {}'.format(*___)
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
