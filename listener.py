#!/usr/bin/env python3
"""
PICO Robot - Simple Performance Monitor
Replacement for your original telemetry viewer with graphs
"""

import socket
import struct
import time
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
from collections import deque

UDP_IP = "0.0.0.0"
UDP_PORT = 4321


SAMPLES = 100  # Show last 100 readings


time_data = deque(maxlen=SAMPLES)
left_speed = deque(maxlen=SAMPLES)
right_speed = deque(maxlen=SAMPLES)


sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((UDP_IP, UDP_PORT))
sock.setblocking(False)  # Non-blocking for smooth graphs

print(f"PICO Robot - Performance Monitor")
print(f"Listening on {UDP_IP}:{UDP_PORT}")
print(f"Showing last {SAMPLES} samples")
print(f"Close the graph window to stop")
print("-" * 50)

def receive_data():
    try:
        data, addr = sock.recvfrom(1024)
        
        if len(data) == 8:
            left_raw, right_raw = struct.unpack('ff', data)
            
            left = left_raw
            right = -right_raw
            
            # Store for graphing
            time_data.append(time.time())
            left_speed.append(left)
            right_speed.append(right)

    except socket.error:
        pass

fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
fig.suptitle('PICO Robot - Real-Time Performance')

def animate(frame):
  
    receive_data()
    
    if len(time_data) < 2:
        return
    
    # Convert to relative time (seconds from start)
    t0 = time_data[0]
    t = [x - t0 for x in time_data]
    
    ax1.clear()
    ax1.plot(t, list(left_speed), 'b-', label='Left Motor', linewidth=2)
    ax1.plot(t, list(right_speed), 'r-', label='Right Motor', linewidth=2)
    ax1.set_ylabel('Speed (rad/s)')
    ax1.set_title('Wheel Speeds')
    ax1.legend(loc='upper right')
    ax1.grid(True, alpha=0.3)
    ax1.axhline(y=0, color='k', linestyle='-', alpha=0.2)
    
    ax2.clear()
    # Convert rad/s to RPM: multiply by (60/(2π))
    left_rpm = [s * (60/(2*np.pi)) for s in left_speed]
    right_rpm = [s * (60/(2*np.pi)) for s in right_speed]
    
    ax2.plot(t, left_rpm, 'b-', label='Left Motor', linewidth=2)
    ax2.plot(t, right_rpm, 'r-', label='Right Motor', linewidth=2)
    ax2.set_xlabel('Time (s)')
    ax2.set_ylabel('RPM')
    ax2.set_title('Motor RPM')
    ax2.legend(loc='upper right')
    ax2.grid(True, alpha=0.3)
    ax2.axhline(y=0, color='k', linestyle='-', alpha=0.2)
    
    if len(left_speed) > 0:
        last_left = left_speed[-1]
        last_right = right_speed[-1]
        last_left_rpm = last_left * (60/(2*np.pi))
        last_right_rpm = last_right * (60/(2*np.pi))
        
        # Direction indicator
        if abs(last_left) < 0.1 and abs(last_right) < 0.1:
            direction = "STOPPED"
        elif last_left > 0 and last_right > 0:
            direction = "FORWARD ↑"
        elif last_left < 0 and last_right < 0:
            direction = "BACKWARD ↓"
        elif last_left > last_right:
            direction = "TURNING RIGHT →"
        else:
            direction = "TURNING LEFT ←"
        
        fig.suptitle(f'PICO Robot - {direction} | L:{last_left:.2f} rad/s ({last_left_rpm:.0f} RPM)  R:{last_right:.2f} rad/s ({last_right_rpm:.0f} RPM)')

ani = FuncAnimation(fig, animate, interval=100)  # Update every 100ms
plt.tight_layout()
plt.show()

print("\nViewer stopped.")
print(f"Last {len(time_data)} samples displayed")
sock.close()