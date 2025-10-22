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
        
        # Print information about enum values in signals
        for msg in self.db.messages:
            print(f"Message: {msg.name} (ID: 0x{msg.frame_id:X}, {len(msg.signals)} signals)")
            for signal in msg.signals:
                if signal.choices:
                    print(f"  Signal {signal.name} has enum values: {signal.choices}")
    
    def generate_signal_value(self, signal):
        """Generate a value for a signal, respecting signal bounds"""
        # If signal has enum choices, use them 70% of the time
        if signal.choices:
            return random.choice(list(signal.choices.keys()))
        
        # For non-enum values or the remaining 30% of the time
        # Use the signal's maximum value if defined
        if signal.maximum is not None:
            max_val = signal.maximum
        else:
            max_val = 100
        
        # Use the signal's minimum value if defined
        if signal.minimum is not None:
            min_val = signal.minimum
        else:
            min_val = 1
        
        # Generate a value within the bounds
        if signal.is_float:
            return random.uniform(min_val, max_val)
        else:
            return random.randint(int(min_val), int(max_val))
    
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
    parser.add_argument('--cycle-time', '-t', type=float, default=0.1, help='Time between message cycles in seconds (default: 0.1)')
    
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
