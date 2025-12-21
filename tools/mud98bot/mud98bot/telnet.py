"""
Telnet protocol handler for Mud98Bot.

Handles telnet option negotiation (IAC sequences) and extracts clean text.
"""

import logging
from typing import Tuple, Callable, Optional
from dataclasses import dataclass, field
from enum import IntEnum

logger = logging.getLogger(__name__)


class TelnetCommand(IntEnum):
    """Telnet command codes."""
    SE = 240     # Subnegotiation End
    NOP = 241    # No Operation
    DM = 242     # Data Mark
    BRK = 243    # Break
    IP = 244     # Interrupt Process
    AO = 245     # Abort Output
    AYT = 246    # Are You There
    EC = 247     # Erase Character
    EL = 248     # Erase Line
    GA = 249     # Go Ahead
    SB = 250     # Subnegotiation Begin
    WILL = 251   # Will do option
    WONT = 252   # Won't do option
    DO = 253     # Request to do option
    DONT = 254   # Request to not do option
    IAC = 255    # Interpret As Command


class TelnetOption(IntEnum):
    """Telnet option codes."""
    ECHO = 1           # Echo
    SGA = 3            # Suppress Go-Ahead
    TTYPE = 24         # Terminal Type
    EOR = 25           # End of Record
    NAWS = 31          # Negotiate About Window Size
    NEW_ENVIRON = 39   # New Environment Option
    CHARSET = 42       # Character Set
    MSDP = 69          # MUD Server Data Protocol
    MSSP = 70          # MUD Server Status Protocol
    MCCP2 = 86         # MUD Client Compression Protocol v2
    MCCP3 = 87         # MUD Client Compression Protocol v3
    MSP = 90           # MUD Sound Protocol
    MXP = 91           # MUD Extension Protocol
    GMCP = 201         # Generic MUD Communication Protocol


# MSDP special characters
class MSDPCode(IntEnum):
    """MSDP protocol codes."""
    VAR = 1
    VAL = 2
    TABLE_OPEN = 3
    TABLE_CLOSE = 4
    ARRAY_OPEN = 5
    ARRAY_CLOSE = 6


@dataclass
class TelnetEvent:
    """Represents a telnet protocol event."""
    command: TelnetCommand
    option: Optional[TelnetOption] = None
    subneg_data: bytes = b""


@dataclass
class TelnetState:
    """Tracks enabled telnet options."""
    echo_enabled: bool = False
    sga_enabled: bool = False
    eor_enabled: bool = False
    msdp_enabled: bool = False
    gmcp_enabled: bool = False
    mccp2_enabled: bool = False
    naws_sent: bool = False


class TelnetHandler:
    """
    Handles telnet protocol negotiation.
    
    Parses incoming data, extracts IAC sequences, and produces clean text.
    Generates appropriate responses to telnet option requests.
    """
    
    def __init__(self):
        self.state = TelnetState()
        self._partial_iac: bytes = b""
        
        # Callbacks for subnegotiation data
        self._msdp_callback: Optional[Callable[[bytes], None]] = None
        self._gmcp_callback: Optional[Callable[[bytes], None]] = None
        
    def set_msdp_callback(self, callback: Callable[[bytes], None]) -> None:
        """Set callback for MSDP subnegotiation data."""
        self._msdp_callback = callback
        
    def set_gmcp_callback(self, callback: Callable[[bytes], None]) -> None:
        """Set callback for GMCP subnegotiation data."""
        self._gmcp_callback = callback
    
    def process(self, data: bytes) -> Tuple[bytes, bytes, list[TelnetEvent]]:
        """
        Process incoming data, separating telnet commands from text.
        
        Args:
            data: Raw bytes from server
            
        Returns:
            Tuple of (clean_text, response_bytes, events)
            - clean_text: Text with telnet sequences removed
            - response_bytes: Telnet responses to send back
            - events: List of telnet events that occurred
        """
        # Prepend any partial IAC from previous call
        if self._partial_iac:
            data = self._partial_iac + data
            self._partial_iac = b""
        
        clean_text = bytearray()
        responses = bytearray()
        events: list[TelnetEvent] = []
        
        i = 0
        while i < len(data):
            byte = data[i]
            
            if byte == TelnetCommand.IAC:
                # Start of telnet sequence
                if i + 1 >= len(data):
                    # Incomplete sequence, save for next call
                    self._partial_iac = data[i:]
                    break
                    
                next_byte = data[i + 1]
                
                if next_byte == TelnetCommand.IAC:
                    # Escaped IAC (literal 255)
                    clean_text.append(TelnetCommand.IAC)
                    i += 2
                    
                elif next_byte in (TelnetCommand.WILL, TelnetCommand.WONT, 
                                   TelnetCommand.DO, TelnetCommand.DONT):
                    if i + 2 >= len(data):
                        self._partial_iac = data[i:]
                        break
                        
                    option = data[i + 2]
                    event, response = self._handle_negotiation(next_byte, option)
                    if event:
                        events.append(event)
                    if response:
                        responses.extend(response)
                    i += 3
                    
                elif next_byte == TelnetCommand.SB:
                    # Subnegotiation - find SE
                    se_idx = self._find_subneg_end(data, i + 2)
                    if se_idx == -1:
                        # Incomplete, save for next call
                        self._partial_iac = data[i:]
                        break
                        
                    subneg_data = data[i + 2:se_idx]
                    event = self._handle_subnegotiation(subneg_data)
                    if event:
                        events.append(event)
                    i = se_idx + 2  # Skip IAC SE
                    
                elif next_byte in (TelnetCommand.GA, TelnetCommand.NOP, 
                                   TelnetCommand.EL, TelnetCommand.EC):
                    # Simple commands, just record event
                    events.append(TelnetEvent(command=TelnetCommand(next_byte)))
                    i += 2
                    
                else:
                    # Unknown command, skip
                    logger.debug(f"Unknown telnet command: {next_byte}")
                    i += 2
            else:
                # Regular character
                clean_text.append(byte)
                i += 1
                
        return bytes(clean_text), bytes(responses), events
    
    def _find_subneg_end(self, data: bytes, start: int) -> int:
        """Find the position of IAC SE in subnegotiation."""
        i = start
        while i < len(data) - 1:
            if data[i] == TelnetCommand.IAC:
                if data[i + 1] == TelnetCommand.SE:
                    return i
                elif data[i + 1] == TelnetCommand.IAC:
                    i += 2  # Skip escaped IAC
                    continue
            i += 1
        return -1
    
    def _handle_negotiation(self, command: int, option: int) -> Tuple[Optional[TelnetEvent], bytes]:
        """Handle WILL/WONT/DO/DONT negotiation."""
        event = TelnetEvent(
            command=TelnetCommand(command),
            option=TelnetOption(option) if option in TelnetOption._value2member_map_ else None
        )
        
        response = b""
        opt_name = TelnetOption(option).name if option in TelnetOption._value2member_map_ else str(option)
        
        if command == TelnetCommand.WILL:
            # Server offers to enable option
            logger.debug(f"Server WILL {opt_name}")
            
            if option == TelnetOption.ECHO:
                # Accept echo (server will echo)
                response = bytes([TelnetCommand.IAC, TelnetCommand.DO, option])
                self.state.echo_enabled = True
                
            elif option == TelnetOption.SGA:
                # Accept suppress go-ahead
                response = bytes([TelnetCommand.IAC, TelnetCommand.DO, option])
                self.state.sga_enabled = True
                
            elif option == TelnetOption.EOR:
                # Accept end-of-record
                response = bytes([TelnetCommand.IAC, TelnetCommand.DO, option])
                self.state.eor_enabled = True
                
            elif option == TelnetOption.MSDP:
                # Accept MSDP
                response = bytes([TelnetCommand.IAC, TelnetCommand.DO, option])
                self.state.msdp_enabled = True
                logger.info("MSDP enabled")
                
            elif option == TelnetOption.GMCP:
                # Accept GMCP
                response = bytes([TelnetCommand.IAC, TelnetCommand.DO, option])
                self.state.gmcp_enabled = True
                logger.info("GMCP enabled")
                
            elif option == TelnetOption.MCCP2:
                # For now, decline compression to keep things simple
                response = bytes([TelnetCommand.IAC, TelnetCommand.DONT, option])
                logger.debug("Declining MCCP2")
                
            else:
                # Decline unknown options
                response = bytes([TelnetCommand.IAC, TelnetCommand.DONT, option])
                
        elif command == TelnetCommand.DO:
            # Server requests we enable option
            logger.debug(f"Server DO {opt_name}")
            
            if option == TelnetOption.NAWS:
                # Send window size
                response = bytes([TelnetCommand.IAC, TelnetCommand.WILL, option])
                # Follow with subnegotiation
                response += self._build_naws_subneg(80, 24)
                self.state.naws_sent = True
                
            elif option == TelnetOption.TTYPE:
                # Send terminal type
                response = bytes([TelnetCommand.IAC, TelnetCommand.WILL, option])
                
            else:
                # Decline
                response = bytes([TelnetCommand.IAC, TelnetCommand.WONT, option])
                
        elif command == TelnetCommand.WONT:
            logger.debug(f"Server WONT {opt_name}")
            # Server declined, update state
            if option == TelnetOption.MSDP:
                self.state.msdp_enabled = False
            elif option == TelnetOption.GMCP:
                self.state.gmcp_enabled = False
                
        elif command == TelnetCommand.DONT:
            logger.debug(f"Server DONT {opt_name}")
            
        return event, response
    
    def _handle_subnegotiation(self, data: bytes) -> Optional[TelnetEvent]:
        """Handle subnegotiation data."""
        if len(data) < 1:
            return None
            
        option = data[0]
        subneg_data = data[1:]
        
        if option == TelnetOption.MSDP:
            logger.debug(f"MSDP subneg: {subneg_data!r}")
            if self._msdp_callback:
                self._msdp_callback(subneg_data)
            return TelnetEvent(
                command=TelnetCommand.SB,
                option=TelnetOption.MSDP,
                subneg_data=subneg_data
            )
            
        elif option == TelnetOption.GMCP:
            logger.debug(f"GMCP subneg: {subneg_data!r}")
            if self._gmcp_callback:
                self._gmcp_callback(subneg_data)
            return TelnetEvent(
                command=TelnetCommand.SB,
                option=TelnetOption.GMCP,
                subneg_data=subneg_data
            )
            
        elif option == TelnetOption.TTYPE:
            # Terminal type request
            if subneg_data == bytes([1]):  # SEND
                logger.debug("Terminal type requested")
                # We'd send response here
                
        return TelnetEvent(
            command=TelnetCommand.SB,
            option=TelnetOption(option) if option in TelnetOption._value2member_map_ else None,
            subneg_data=subneg_data
        )
    
    def _build_naws_subneg(self, width: int, height: int) -> bytes:
        """Build NAWS subnegotiation to send window size."""
        # IAC SB NAWS <width-high> <width-low> <height-high> <height-low> IAC SE
        w_high = (width >> 8) & 0xFF
        w_low = width & 0xFF
        h_high = (height >> 8) & 0xFF
        h_low = height & 0xFF
        
        return bytes([
            TelnetCommand.IAC, TelnetCommand.SB, TelnetOption.NAWS,
            w_high, w_low, h_high, h_low,
            TelnetCommand.IAC, TelnetCommand.SE
        ])
    
    def build_msdp_request(self, command: str, *variables: str) -> bytes:
        """
        Build an MSDP request.
        
        Args:
            command: MSDP command (SEND, REPORT, etc.)
            variables: Variable names to request
            
        Returns:
            Complete telnet sequence to send
        """
        data = bytearray([TelnetCommand.IAC, TelnetCommand.SB, TelnetOption.MSDP])
        
        # Command
        data.append(MSDPCode.VAR)
        data.extend(command.encode('utf-8'))
        
        # Variables
        for var in variables:
            data.append(MSDPCode.VAL)
            data.extend(var.encode('utf-8'))
            
        data.extend([TelnetCommand.IAC, TelnetCommand.SE])
        return bytes(data)
    
    def build_msdp_report(self, *variables: str) -> bytes:
        """Build MSDP REPORT command to subscribe to variable updates."""
        return self.build_msdp_request("REPORT", *variables)
    
    def build_msdp_send(self, *variables: str) -> bytes:
        """Build MSDP SEND command to request current values."""
        return self.build_msdp_request("SEND", *variables)


def strip_ansi(text: bytes) -> str:
    """
    Remove ANSI escape sequences from text.
    
    Args:
        text: Raw bytes possibly containing ANSI codes
        
    Returns:
        Clean string with ANSI codes removed
    """
    import re
    decoded = text.decode('utf-8', errors='replace')
    # Remove ANSI escape sequences
    ansi_escape = re.compile(r'\x1b\[[0-9;]*[a-zA-Z]')
    return ansi_escape.sub('', decoded)
