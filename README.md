# TVSign
controller and communication for TV LED sign

## Bluetooth communication

Bluetooth commands are sent as 2-byte codes.  
The 2 bytes of the command should be sent in a
single transmission
For some commands, additional bytes may be sent.

See send_cmd.py for the python implementation of the bluetooth communication

### Commands

* next pattern : 0x4a, 0x01
* next speed : 0x4a, 0x02
* next brightness : 0x4a, 0x03
* toggle auto update : 0x4a, 0x04.  By default the patterns automatically update after some time.  Use this command to disable/enable the automatic update
* change color : 0xa4, `colorID`. Followed by 3 bytes.  The colorID should be values of 1, 2, 3,or 4, each corresponding to a color.  1 = violet, 2 = cyan, 3 = yellow, 4 = beige. After the command is received, an acknowledgement bit is returned. Following the reception of the acknowledgemet, 3 additional bytes should be sent corresponding to the R, G, B values of the new color
* race length : 0xa5, `length` . The second byte should be the desired length
* sparkle count: 0xa6, `count`. The second byte should be the desired count
