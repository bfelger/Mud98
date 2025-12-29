"""
Connection manager for Mud98Bot.

Handles raw TCP/TLS socket communication with the MUD server.
"""

import socket
import ssl
import logging
from typing import Optional
from dataclasses import dataclass
from enum import Enum, auto

logger = logging.getLogger(__name__)


class ConnectionState(Enum):
    """Connection state machine states."""
    DISCONNECTED = auto()
    CONNECTING = auto()
    CONNECTED = auto()
    ERROR = auto()


@dataclass
class ConnectionConfig:
    """Configuration for server connection."""
    host: str = "localhost"
    port: int = 4000
    use_tls: bool = False
    timeout: float = 30.0
    buffer_size: int = 4096


class Connection:
    """
    Manages the raw socket connection to a MUD server.
    
    Handles:
    - TCP socket creation and connection
    - Optional TLS/SSL wrapping
    - Buffered reading and writing
    - Connection state tracking
    - Timeout handling
    """
    
    def __init__(self, config: Optional[ConnectionConfig] = None):
        self.config = config or ConnectionConfig()
        self._socket: Optional[socket.socket] = None
        self._state = ConnectionState.DISCONNECTED
        self._read_buffer = bytearray()
        
    @property
    def state(self) -> ConnectionState:
        return self._state
    
    @property
    def is_connected(self) -> bool:
        return self._state == ConnectionState.CONNECTED
    
    def connect(self) -> bool:
        """
        Establish connection to the MUD server.
        
        Returns:
            True if connection successful, False otherwise.
        """
        if self._state == ConnectionState.CONNECTED:
            logger.warning("Already connected")
            return True
            
        self._state = ConnectionState.CONNECTING
        
        try:
            # Create TCP socket
            self._socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self._socket.settimeout(self.config.timeout)
            
            logger.info(f"Connecting to {self.config.host}:{self.config.port}...")
            self._socket.connect((self.config.host, self.config.port))
            
            # Wrap with TLS if configured
            if self.config.use_tls:
                logger.info("Upgrading to TLS...")
                context = ssl.create_default_context()
                # For testing, we may need to disable cert verification
                context.check_hostname = False
                context.verify_mode = ssl.CERT_NONE
                self._socket = context.wrap_socket(
                    self._socket, 
                    server_hostname=self.config.host
                )
            
            self._state = ConnectionState.CONNECTED
            logger.info("Connected successfully")
            return True
            
        except socket.timeout:
            logger.error(f"Connection timed out after {self.config.timeout}s")
            self._state = ConnectionState.ERROR
            return False
            
        except socket.error as e:
            logger.error(f"Socket error: {e}")
            self._state = ConnectionState.ERROR
            return False
            
        except ssl.SSLError as e:
            logger.error(f"TLS error: {e}")
            self._state = ConnectionState.ERROR
            return False
    
    def disconnect(self) -> None:
        """Close the connection gracefully."""
        if self._socket:
            try:
                self._socket.shutdown(socket.SHUT_RDWR)
            except socket.error:
                pass  # Already disconnected
            finally:
                self._socket.close()
                self._socket = None
                
        self._state = ConnectionState.DISCONNECTED
        self._read_buffer.clear()
        logger.info("Disconnected")
    
    def send(self, data: bytes) -> bool:
        """
        Send raw bytes to the server.
        
        Args:
            data: Bytes to send
            
        Returns:
            True if send successful, False otherwise
        """
        if not self.is_connected or not self._socket:
            logger.error("Cannot send: not connected")
            return False
            
        try:
            self._socket.sendall(data)
            logger.debug(f"Sent {len(data)} bytes: {data!r}")
            return True
            
        except socket.error as e:
            logger.error(f"Send error: {e}")
            self._state = ConnectionState.ERROR
            return False
    
    def send_line(self, text: str) -> bool:
        """
        Send a line of text to the server (adds \\r\\n).
        
        Args:
            text: Text to send (without newline)
            
        Returns:
            True if send successful
        """
        return self.send((text + "\r\n").encode('utf-8'))
    
    def recv(self, timeout: Optional[float] = None) -> bytes:
        """
        Receive available data from the server.
        
        Args:
            timeout: Optional timeout override
            
        Returns:
            Received bytes (may be empty if timeout)
        """
        if not self.is_connected or not self._socket:
            return b""
            
        old_timeout = self._socket.gettimeout()
        
        try:
            if timeout is not None:
                self._socket.settimeout(timeout)
                
            data = self._socket.recv(self.config.buffer_size)
            
            if not data:
                # Connection closed by server
                logger.info("Server closed connection")
                self._state = ConnectionState.DISCONNECTED
                return b""
                
            logger.debug(f"Received {len(data)} bytes")
            return data
            
        except socket.timeout:
            return b""
            
        except socket.error as e:
            logger.error(f"Recv error: {e}")
            self._state = ConnectionState.ERROR
            return b""
            
        finally:
            if timeout is not None:
                self._socket.settimeout(old_timeout)
    
    def recv_until(self, marker: bytes, timeout: float = 10.0) -> bytes:
        """
        Receive data until a marker is found.
        
        Args:
            marker: Bytes sequence to wait for
            timeout: Maximum time to wait
            
        Returns:
            All data received including marker, or empty if timeout
        """
        import time
        start = time.time()
        
        while time.time() - start < timeout:
            # Check if marker is already in buffer
            if marker in self._read_buffer:
                idx = self._read_buffer.index(marker) + len(marker)
                result = bytes(self._read_buffer[:idx])
                self._read_buffer = self._read_buffer[idx:]
                return result
            
            # Read more data
            remaining = timeout - (time.time() - start)
            if remaining <= 0:
                break
                
            data = self.recv(timeout=min(remaining, 1.0))
            if data:
                self._read_buffer.extend(data)
            elif self._state != ConnectionState.CONNECTED:
                break
                
        # Timeout - return what we have
        result = bytes(self._read_buffer)
        self._read_buffer.clear()
        return result
    
    def set_blocking(self, blocking: bool) -> None:
        """Set socket blocking mode."""
        if self._socket:
            self._socket.setblocking(blocking)
    
    def __enter__(self):
        self.connect()
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
        return False
