"""
Bot client for Mud98Bot.

High-level client that combines connection, telnet, and MSDP handling.
Manages login flow and game state.
"""

import logging
import time
import re
from typing import Optional, Callable
from dataclasses import dataclass, field
from enum import Enum, auto

from .connection import Connection, ConnectionConfig, ConnectionState
from .telnet import TelnetHandler, TelnetOption, strip_ansi
from .msdp import MSDPParser, CharacterStats, RoomInfo
from .parser import TextParser, BotRoomData

logger = logging.getLogger(__name__)


class BotState(Enum):
    """High-level bot state machine."""
    DISCONNECTED = auto()
    CONNECTING = auto()
    AWAITING_NAME = auto()
    AWAITING_PASSWORD = auto()
    CONFIRMING_NAME = auto()       # New character
    CREATING_CHARACTER = auto()    # Character creation flow
    AWAITING_MOTD = auto()
    PLAYING = auto()
    ERROR = auto()


@dataclass
class BotConfig:
    """Bot configuration."""
    # Connection
    host: str = "localhost"
    port: int = 4000
    use_tls: bool = False
    timeout: float = 30.0
    
    # Credentials
    username: str = ""
    password: str = ""
    
    # New character defaults
    new_char_race: str = "human"
    new_char_class: str = "warrior"
    new_char_sex: str = "male"
    new_char_alignment: str = "neutral"
    
    # Throttling
    min_command_delay: float = 0.25
    
    # Behavior
    auto_subscribe_msdp: bool = True


# Direction commands for VNUM transition logging
DIRECTION_COMMANDS = {
    'north', 'south', 'east', 'west', 'up', 'down',
    'n', 's', 'e', 'w', 'u', 'd'
}


class Bot:
    """
    High-level MUD bot client.
    
    Manages the full lifecycle from connection through gameplay.
    """
    
    def __init__(self, config: Optional[BotConfig] = None):
        self.config = config or BotConfig()
        
        # Per-bot logger with username in the name
        bot_name = self.config.username or "unnamed"
        self._logger = logging.getLogger(f'mud98bot.client({bot_name})')
        
        # Components
        self._conn = Connection(ConnectionConfig(
            host=self.config.host,
            port=self.config.port,
            use_tls=self.config.use_tls,
            timeout=self.config.timeout
        ))
        self._telnet = TelnetHandler()
        self._msdp = MSDPParser()
        
        # Wire up MSDP callback
        self._telnet.set_msdp_callback(self._on_msdp_data)
        
        # State
        self._state = BotState.DISCONNECTED
        self._last_command_time: float = 0
        self._text_buffer: str = ""
        self._prompt_detected: bool = False
        
        # Bot protocol data (PLR_BOT structured output)
        self._parser = TextParser()
        self._bot_data: BotRoomData = BotRoomData()
        self._bot_mode: bool = False  # Set True if we detect BOT protocol output
        
        # Callbacks
        self._text_callbacks: list[Callable[[str], None]] = []
        self._prompt_callbacks: list[Callable[[str], None]] = []
        
    @property
    def state(self) -> BotState:
        return self._state
    
    @property
    def bot_id(self) -> str:
        """Return the bot identifier (username)."""
        return self.config.username or "unnamed"
    
    @property
    def is_playing(self) -> bool:
        return self._state == BotState.PLAYING
    
    @property
    def stats(self) -> CharacterStats:
        return self._msdp.stats
    
    @property
    def room(self) -> RoomInfo:
        return self._msdp.room
    
    @property
    def bot_data(self) -> BotRoomData:
        """Current room data from BOT protocol."""
        return self._bot_data
    
    @property
    def bot_mode(self) -> bool:
        """True if PLR_BOT structured output has been detected."""
        return self._bot_mode
    
    def add_text_callback(self, callback: Callable[[str], None]) -> None:
        """Add callback for received text."""
        self._text_callbacks.append(callback)
        
    def add_prompt_callback(self, callback: Callable[[str], None]) -> None:
        """Add callback for prompt detection."""
        self._prompt_callbacks.append(callback)
    
    def connect(self) -> bool:
        """Connect to the MUD server."""
        self._state = BotState.CONNECTING
        
        if not self._conn.connect():
            self._state = BotState.ERROR
            return False
            
        self._state = BotState.AWAITING_NAME
        return True
    
    def disconnect(self) -> None:
        """Disconnect from the server."""
        self._conn.disconnect()
        self._state = BotState.DISCONNECTED
    
    def login(self, timeout: float = 30.0) -> bool:
        """
        Perform the full login sequence.
        
        Returns:
            True if login successful and now playing
        """
        if not self._conn.is_connected:
            if not self.connect():
                return False
        
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            # Read and process data
            data = self._conn.recv(timeout=1.0)
            if data:
                self._process_data(data)
            
            # Check for state transitions from text BEFORE acting
            self._check_login_transitions()
            
            # State machine
            if self._state == BotState.AWAITING_NAME:
                if self._check_text("what name", "by what name", "your name"):
                    self._send_line(self.config.username)
                    self._clear_buffer()
                    # State will be updated based on response
                    
            elif self._state == BotState.AWAITING_PASSWORD:
                if self._check_text("password:"):
                    self._send_line(self.config.password)
                    self._clear_buffer()
                    
            elif self._state == BotState.CONFIRMING_NAME:
                # New character - "Did I get that right"
                if self._check_text("did i get that right"):
                    self._send_line("y")
                    self._clear_buffer()
                    self._state = BotState.CREATING_CHARACTER
                    
            elif self._state == BotState.CREATING_CHARACTER:
                # Handle character creation prompts
                if not self._handle_character_creation():
                    continue
                    
            elif self._state == BotState.AWAITING_MOTD:
                if self._check_text("press enter", "[hit return", "continue]"):
                    self._send_line("")
                    self._clear_buffer()
                    self._state = BotState.PLAYING
                    
            elif self._state == BotState.PLAYING:
                self._logger.info("Login successful - now playing")
                
                # Subscribe to MSDP updates if enabled
                if self.config.auto_subscribe_msdp and self._telnet.state.msdp_enabled:
                    self._subscribe_msdp()
                
                # Send initial 'look' to get room data for BOT protocol
                self._send_line("look")
                # Wait a moment for response and process it
                time.sleep(0.3)
                data = self._conn.recv(timeout=1.0)
                if data:
                    self._process_data(data)
                    
                return True
                
            elif self._state == BotState.ERROR:
                self._logger.error("Login failed - error state")
                return False
            
        self._logger.error(f"Login timeout after {timeout}s")
        return False
    
    def _process_data(self, data: bytes) -> None:
        """Process incoming data through telnet handler."""
        clean_text, response, events = self._telnet.process(data)
        
        # Send any telnet responses
        if response:
            self._conn.send(response)
        
        # Convert to string and buffer
        if clean_text:
            text = strip_ansi(clean_text)
            self._text_buffer += text
            
            # Check for BOT protocol data
            if self._parser.has_bot_data(text):
                self._bot_mode = True
                self._bot_data = self._parser.parse_bot_data(text)
                self._logger.debug(f"BOT data parsed: room={self._bot_data.room}, "
                           f"exits={len(self._bot_data.exits)}, "
                           f"mobs={len(self._bot_data.mobs)}, "
                           f"objects={len(self._bot_data.objects)}")
            
            # Notify callbacks
            for callback in self._text_callbacks:
                try:
                    callback(text)
                except Exception as e:
                    self._logger.error(f"Text callback error: {e}")
            
            # Check for prompt
            if self._detect_prompt(text):
                self._prompt_detected = True
                for callback in self._prompt_callbacks:
                    try:
                        callback(text)
                    except Exception as e:
                        self._logger.error(f"Prompt callback error: {e}")
    
    def _on_msdp_data(self, data: bytes) -> None:
        """Handle MSDP subnegotiation data."""
        old_vnum = self._msdp.stats.room_vnum
        self._msdp.parse(data)
        new_vnum = self._msdp.stats.room_vnum
        
        # On room change, clear stale bot_data (will be repopulated on next 'look')
        if new_vnum != old_vnum and old_vnum != 0:
            self._logger.debug(f"ROOM: {old_vnum} -> {new_vnum}")
            # Clear bot_data to prevent stale mob/object info
            self._bot_data = BotRoomData()
    
    def _detect_prompt(self, text: str) -> bool:
        """Check if text contains a prompt."""
        # Common MUD prompt patterns
        prompt_patterns = [
            r'<\d+hp \d+m \d+mv>',  # <100hp 50m 200mv>
            r'\[\d+/\d+hp \d+/\d+m \d+/\d+mv\]',  # [100/100hp ...]
            r'>\s*$',  # Simple > prompt
        ]
        
        for pattern in prompt_patterns:
            if re.search(pattern, text):
                return True
        return False
    
    def _check_text(self, *patterns: str) -> bool:
        """Check if any pattern appears in the text buffer (case-insensitive)."""
        buffer_lower = self._text_buffer.lower()
        for pattern in patterns:
            if pattern.lower() in buffer_lower:
                return True
        return False
    
    def _check_login_transitions(self) -> None:
        """Check text buffer for login state transitions."""
        if self._state == BotState.AWAITING_NAME:
            if self._check_text("password:"):
                self._state = BotState.AWAITING_PASSWORD
                # Don't clear buffer - action handler needs to see "password:"
            elif self._check_text("did i get that right"):
                self._state = BotState.CONFIRMING_NAME
        elif self._state == BotState.AWAITING_PASSWORD:
            if self._check_text("hit return to continue", "press enter", "[hit return", "message of the day"):
                self._state = BotState.AWAITING_MOTD
                # Don't clear buffer - action handler needs to detect MOTD prompts
            elif self._check_text("reconnecting"):
                # Character was already logged in - we're now playing
                self._state = BotState.PLAYING
            elif self._check_text("wrong password"):
                self._state = BotState.ERROR
                self._logger.error("Wrong password")
    
    def _handle_character_creation(self) -> bool:
        """Handle character creation prompts. Returns True when done."""
        text = self._text_buffer.lower()
        
        if "select a race" in text or "choose a race" in text or "the following races" in text:
            self._send_line(self.config.new_char_race)
            self._clear_buffer()
            return False
            
        if "select a class" in text or "choose a class" in text or "the following classes" in text:
            self._send_line(self.config.new_char_class)
            self._clear_buffer()
            return False
            
        if "sex" in text and ("male" in text or "female" in text):
            self._send_line(self.config.new_char_sex)
            self._clear_buffer()
            return False
            
        if "alignment" in text and ("good" in text or "neutral" in text or "evil" in text):
            self._send_line(self.config.new_char_alignment)
            self._clear_buffer()
            return False
            
        if "give me a password" in text:
            self._send_line(self.config.password)
            self._clear_buffer()
            return False
            
        if "retype password" in text or "please retype" in text:
            self._send_line(self.config.password)
            self._clear_buffer()
            return False
            
        if "customize" in text or "do you wish to customize" in text:
            # Accept defaults
            self._send_line("n")
            self._clear_buffer()
            return False
            
        if "pick a weapon" in text or "weapon from the following" in text:
            # Pick a weapon - usually sword or first option
            self._send_line("sword")
            self._clear_buffer()
            return False
            
        if "press enter" in text or "motd" in text or "[hit return" in text:
            self._state = BotState.AWAITING_MOTD
            return True
            
        return False
    
    def _clear_buffer(self) -> None:
        """Clear the text buffer."""
        self._text_buffer = ""
    
    def _send_line(self, text: str) -> bool:
        """Send a line of text, respecting throttle."""
        now = time.time()
        elapsed = now - self._last_command_time
        
        if elapsed < self.config.min_command_delay:
            time.sleep(self.config.min_command_delay - elapsed)
            
        self._last_command_time = time.time()
        return self._conn.send_line(text)
    
    def _subscribe_msdp(self) -> None:
        """Subscribe to MSDP variable updates."""
        variables = [
            "HEALTH", "HEALTH_MAX",
            "MANA", "MANA_MAX",
            "MOVEMENT", "MOVEMENT_MAX",
            "LEVEL", "EXPERIENCE",
            "ALIGNMENT", "MONEY",
            "ROOM_EXITS", "ROOM_VNUM",
            "POSITION",  # Character position (standing, sitting, resting, sleeping, fighting)
            "IN_COMBAT", "OPPONENT_NAME",
            "OPPONENT_LEVEL", "OPPONENT_HEALTH", "OPPONENT_HEALTH_MAX"
        ]
        
        self._logger.info(f"Subscribing to MSDP: {variables}")
        request = self._telnet.build_msdp_report(*variables)
        self._conn.send(request)
    
    def send_command(self, command: str) -> bool:
        """Send a game command."""
        if not self.is_playing:
            self._logger.warning("Cannot send command: not playing")
            return False
        
        # For direction commands, show VNUM transition
        cmd_lower = command.lower().split()[0] if command else ""
        if cmd_lower in DIRECTION_COMMANDS:
            current_vnum = self.stats.room_vnum
            self._logger.debug(f"CMD: {command} ({current_vnum}->?)")
        else:
            self._logger.debug(f"CMD: {command}")
        
        return self._send_line(command)
    
    def read_text(self, timeout: float = 1.0) -> str:
        """
        Read and process available text from server.
        
        Returns:
            Received text (ANSI stripped)
        """
        self._clear_buffer()
        
        data = self._conn.recv(timeout=timeout)
        if data:
            self._process_data(data)
            
        result = self._text_buffer
        self._clear_buffer()
        return result
    
    def wait_for_prompt(self, timeout: float = 10.0) -> bool:
        """Wait until a prompt is detected."""
        start = time.time()
        self._prompt_detected = False
        
        while time.time() - start < timeout:
            data = self._conn.recv(timeout=0.5)
            if data:
                self._process_data(data)
                if self._prompt_detected:
                    return True
                    
        return False
    
    def send_and_wait(self, command: str, timeout: float = 10.0) -> str:
        """Send command and wait for prompt, returning output."""
        self._clear_buffer()
        self.send_command(command)
        self.wait_for_prompt(timeout)
        return self._text_buffer
    
    def __enter__(self):
        return self
        
    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()
        return False
