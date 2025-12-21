#!/usr/bin/env python3
"""
Quick connection test for Mud98Bot.

Usage:
    python test_connection.py [host] [port]
    
Example:
    python test_connection.py localhost 4000
"""

import sys
import logging

# Add parent to path for imports
sys.path.insert(0, '.')

from mud98bot.connection import Connection, ConnectionConfig
from mud98bot.telnet import TelnetHandler, strip_ansi

logging.basicConfig(level=logging.DEBUG)


def test_connection(host: str = "localhost", port: int = 4000) -> None:
    """Test basic connection and telnet negotiation."""
    print(f"\n=== Testing connection to {host}:{port} ===\n")
    
    config = ConnectionConfig(host=host, port=port, timeout=10.0)
    conn = Connection(config)
    telnet = TelnetHandler()
    
    if not conn.connect():
        print("FAILED: Could not connect")
        return
        
    print("Connected successfully!")
    
    # Read initial data (should include telnet negotiation and banner)
    print("\n--- Receiving initial data ---")
    
    for i in range(5):  # Read a few chunks
        data = conn.recv(timeout=1.0)
        if not data:
            continue
            
        print(f"Raw bytes ({len(data)}): {data[:100]!r}...")
        
        # Process through telnet handler
        clean, response, events = telnet.process(data)
        
        if response:
            print(f"Sending telnet response ({len(response)} bytes)")
            conn.send(response)
            
        for event in events:
            print(f"Telnet event: {event}")
            
        if clean:
            text = strip_ansi(clean)
            print(f"Clean text:\n{text[:500]}")
    
    print("\n--- Telnet state ---")
    print(f"ECHO: {telnet.state.echo_enabled}")
    print(f"SGA: {telnet.state.sga_enabled}")
    print(f"EOR: {telnet.state.eor_enabled}")
    print(f"MSDP: {telnet.state.msdp_enabled}")
    print(f"GMCP: {telnet.state.gmcp_enabled}")
    
    conn.disconnect()
    print("\nDisconnected. Test complete!")


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 4000
    test_connection(host, port)
