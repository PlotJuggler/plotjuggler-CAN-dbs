#!/usr/bin/env python3
import time
import argparse
import random
import threading
import cantools
import can

class CanSimulator:
    def __init__(self, dbc_file, interface='vcan0', cycle_time=0.1):
        """
        Initialize CAN simulator with a DBC file
        
        Args:
            dbc_file: Path to the DBC file
            interface: CAN interface name
            cycle_time: Time between message cycles in seconds
        """
        self.db = cantools.database.load_file(dbc_file)
        self.bus = can.interface.Bus(channel=interface, bustype='socketcan')
        self.cycle_time = cycle_time
        self.counter = 0
        self.running = False
        self.thread = None
        
        print(f"Loaded {len(self.db.messages)} messages from {dbc_file}")
        for msg in self.db.messages:
            print(f"Message: {msg.name} (ID: 0x{msg.frame_id:X}, {len(msg.signals)} signals)")
    
    def generate_signal_value(self, signal):
        """Generate a random value for a signal based on its properties"""
        if signal.scale == 0:  # Avoid division by zero
            signal.scale = 1
            
        min_val = signal.minimum if signal.minimum is not None else 0
        max_val = signal.maximum if signal.maximum is not None else 100
        
        # If min/max aren't defined or are invalid, create reasonable defaults
        if min_val is None or max_val is None or min_val >= max_val:
            if signal.is_float:
                min_val, max_val = 0.0, 100.0
            else:
                # For boolean signals
                if signal.scale == 1 and signal.offset == 0 and max_val <= 1:
                    return random.randint(0, 1)
                # For other integer signals
                min_val, max_val = 0, 2**(signal.length) - 1
        
        # Generate value based on signal type
        if signal.is_float:
            value = random.uniform(min_val, max_val) 
        else:
            value = random.randint(int(min_val), int(max_val))
            
        # Some signals might be meant to oscillate around a central value
        if "steering" in signal.name.lower() or "angle" in signal.name.lower():
            # Make steering values oscillate around 0
            center = (min_val + max_val) / 2
            amplitude = (max_val - min_val) / 4
            value = center + amplitude * (self.counter % 100) / 50.0 * (1 if self.counter % 200 < 100 else -1)
        
        # For speed-like signals, create a more realistic pattern
        elif "speed" in signal.name.lower() or "velocity" in signal.name.lower():
            # Gradually increase speed then decrease
            pattern_length = 200
            phase = self.counter % pattern_length
            if phase < pattern_length / 2:
                factor = phase / (pattern_length / 2)
            else:
                factor = 2 - phase / (pattern_length / 2)
            value = min_val + (max_val - min_val) * factor * 0.8
            
        return value
    
    def generate_message_data(self, message):
        """Generate data for a message based on its signals"""
        data = {}
        
        for signal in message.signals:
            data[signal.name] = self.generate_signal_value(signal)
            
        return data
    
    def send_all_messages(self):
        """Send all messages from the DBC file"""
        self.counter += 1
        
        for message in self.db.messages:
            try:
                # Generate signal values
                data = self.generate_message_data(message)
                
                # Encode message
                data_bytes = self.db.encode_message(message.frame_id, data)
                
                # Create and send CAN message
                msg = can.Message(
                    arbitration_id=message.frame_id,
                    data=data_bytes,
                    is_extended_id=message.is_extended_frame
                )
                self.bus.send(msg)
                
            except Exception as e:
                print(f"Error encoding/sending message {message.name}: {e}")
    
    def start(self):
        """Start the simulator in a separate thread"""
        if self.running:
            print("Simulator already running")
            return
            
        self.running = True
        self.thread = threading.Thread(target=self._run)
        self.thread.daemon = True
        self.thread.start()
        print(f"Simulator started - sending messages every {self.cycle_time} seconds")
    
    def _run(self):
        """Main simulator loop"""
        while self.running:
            self.send_all_messages()
            time.sleep(self.cycle_time)
    
    def stop(self):
        """Stop the simulator"""
        self.running = False
        if self.thread:
            self.thread.join(timeout=1.0)
        print("Simulator stopped")
    
def main():
    parser = argparse.ArgumentParser(description='CAN bus simulator using DBC file')
    parser.add_argument('dbc_file', help='Path to the DBC file')
    parser.add_argument('--interface', '-i', default='vcan0', help='CAN interface (default: vcan0)')
    parser.add_argument('--cycle-time', '-t', type=float, default=0.25, help='Time between message cycles in seconds (default: 0.1)')
    
    args = parser.parse_args()
    
    simulator = CanSimulator(args.dbc_file, args.interface, args.cycle_time)
    
    try:
        simulator.start()
        print("Press Ctrl+C to stop")
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nStopping simulator...")
    finally:
        simulator.stop()

if __name__ == "__main__":
    main()
