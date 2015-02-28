import virtserial

def test_all():
    testing_commands = [
        ('', '#OK'),
        ('RD,ALL', '#RD,010000000000000000'),
        ('WR,20,12', '#ERR'),
        ('WR,2,5', '#ERR'),
        ('WR,2,1', '#WR,WRONGLINE'),
        ('RD,2', '#RD,2,1'),
        ('RD,3', '#RD,3,0'),
        ('WR,12,1', '#WR,WRONGLINE'),
        ('REL,5,12', '#ERR'),
        ('REL,4,12', '#ERR'),
        ('REL,3,0', '#REL,OK'),
        ('REL,2,1', '#REL,OK'),
        ('ADC,12', '#ERR'),
        ('ADC,5', '#ERR'),
        ('ADC,4', '#ADC,4,0000'),
        ('ADC,4', '#ADC,4,0001'),
        ('IO,GET,MEM', '#IO,GET,MEM,000000000000000000'),
        ('IO,GET,MEM,12', '#IO,GET,MEM,0'),
        ('IO,SET,20,1', '#ERR'),
        ('IO,SET,12,5', '#ERR'),
        ('IO,SET,12,1', '#IO,SET,OK'),
        ('IO,SET,1,1', '#IO,SET,OK'),
        ('IO,GET,MEM', '#IO,GET,MEM,100000000001000000'),
        ('WR,12,1', '#WR,OK'),
        ('RD,ALL', '#RD,x1000000000x000000'),
        ('IO,SET,12,0', '#IO,SET,OK'),
        ('RD,ALL', '#RD,x10000000001000000'),
        ('IO,SET,1,0', '#IO,SET,OK'),
        ('RD,ALL', '#RD,010000000001000000')
        ]

    serial_backend = virtserial.Serial_FW1()

    for command, output in testing_commands:
        command = '$KE,{}\r\n'.format(command) if command else '$KE\r\n'
        assert serial_backend.process(command) == output, '{} -/-> {}'.format(
            command.strip(), output)

    assert serial_backend.process('$KE,AFR,500\r\n') == '#ERR'
    assert serial_backend.process('$KE,AFR,50\r\n') == '#AFR,OK'
