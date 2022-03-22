"""
Send cmmands to the TV LED sign

commands are sent over bluetooh
"""

import bluetooth
import time
import argparse
import datetime
import numpy as np

serverMACAddress = '00:20:12:08:31:18' 
uuid = "94f39d29-7d6d-437d-973b-fba39e49d4ef"

def parse_args():

    parser = argparse.ArgumentParser()

    parser.add_argument('--next_pattern', dest='next_pattern', default=False, action='store_true', help='go to next pattern')
    parser.add_argument('--speed', dest='speed', default=False, action='store_true', help='change speed')
    parser.add_argument('--brightness', dest='brightness', default=False, action='store_true', help = 'change brightness')
    parser.add_argument('--change_color', dest='change_color', default=None, type=int, help='number of color to change (1,2,3,4)')
    parser.add_argument('--color_values', dest='color_values', default=None, help='comma separated list of 3 RGB values')
    parser.add_argument('--race_length', dest='race_length', default=None, type=int, help='set race length')
    parser.add_argument('--sparkle_count', dest='sparkle_count', default=None, type=int, help='set number of sparkles')
    parser.add_argument('--toggle_auto_update', dest='toggle_auto_update', default=False, action='store_true', help='toggle auto update bit')

    return parser.parse_args()

def main(
    next_pattern=False,
    speed=False,
    brightness=False,
    change_color=None,
    color_values=None,
    race_length=None,
    sparkle_count=None,
    toggle_auto_update=False
):
    s = get_bluetooth_service()

    #go to the next pattern
    if next_pattern:
        vals = [0x4a, 0x01]
        s.send(bytes(vals))
    #change speed
    elif speed:
        vals = [0x4a, 0x02]
        s.send(bytes(vals))
    # change brightness
    elif brightness:
        vals = [0x4a, 0x03]
        s.send(bytes(vals))
    # toggle whether patterns
    # update after a while
    elif toggle_auto_update:
        vals = [0x4a, 0x04]
        s.send(bytes(vals))
    # Change the color on a given side
    # to new RGB values
    elif change_color is not None and color_values is not None:
        send_vals = [0xa4, change_color]
        s.send(bytes(send_vals))
        test = s.recv(1)
        color_vals = [int(x) for x in color_values.split(',')]
        s.send(bytes(color_vals))
    # Change the length of LED
    # trains in the race pattern
    elif race_length is not None:
        send_vals = [0xa5, race_length]
        s.send(bytes(send_vals))
    # Change the number of sparkles generated
    # Only effectve between 1 - 20. 
    elif sparkle_count is not None:
        send_vals = [0xa6, sparkle_count]
        s.send(bytes(send_vals))

    print ('close connection')
    s.close()

def get_bluetooth_service():
    """
    Connect to bluetooth 
    and return socket
    """

    service_matches = bluetooth.find_service(uuid=uuid, address=serverMACAddress)
    
    if len(service_matches) == 0:
        print("Couldn't find the SampleServer service.")
        sys.exit(0)

    first_match = service_matches[0]
    port = first_match["port"]
    host = first_match["host"]

    s = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
    s.connect((serverMACAddress, port))

    return s


if __name__ == '__main__':
    main(**vars(parse_args()))
