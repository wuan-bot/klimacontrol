import os

import requests

if __name__ == '__main__':
    raw_devices = os.environ['KLIMA_DEVICES']
    devices = raw_devices.split(',')
    devices = [device.strip() for device in devices]

    for device in devices:
        try:
            response = requests.get(f"http://{device}/api/status")
        except requests.exceptions.RequestException as e:
            print(f"Error: Failed to get status from {device}: {e}")
            continue
        if response.status_code != 200:
            print(f"Error: Failed to get status from {device}")
            continue
        data = response.json()
        if "firmware_version" in data:
            print(f"Device {device} has firmware version: {data['firmware_version']}")
