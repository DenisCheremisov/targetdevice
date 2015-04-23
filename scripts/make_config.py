#!/usr/bin/env python3

import os
import virtserial


iterator = virtserial.iter_main()
port_name = next(iterator)


ORIG_CONFIG = 'conf/targetdevice.yaml'
RESULTED_CONFIG = 'test_targetdevice.yaml'


with open(ORIG_CONFIG) as f:
    config = f.read()
config = config.replace('/dev/pts/3', port_name)
with open(RESULTED_CONFIG, 'w') as f:
    f.write(config)


for void in iterator:
    pass

os.remove(RESULTED_CONFIG)
