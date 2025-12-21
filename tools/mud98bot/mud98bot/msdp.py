"""
MSDP (MUD Server Data Protocol) parser for Mud98Bot.

Parses MSDP data from subnegotiation and maintains variable state.
"""

import logging
from typing import Any, Optional, Callable
from dataclasses import dataclass, field
from enum import IntEnum

logger = logging.getLogger(__name__)


class MSDPCode(IntEnum):
    """MSDP protocol codes."""
    VAR = 1
    VAL = 2
    TABLE_OPEN = 3
    TABLE_CLOSE = 4
    ARRAY_OPEN = 5
    ARRAY_CLOSE = 6


@dataclass
class CharacterStats:
    """Current character statistics from MSDP."""
    health: int = 0
    health_max: int = 0
    mana: int = 0
    mana_max: int = 0
    movement: int = 0
    movement_max: int = 0
    level: int = 0
    experience: int = 0
    experience_max: int = 0
    alignment: int = 0
    money: int = 0
    in_combat: bool = False
    opponent_name: str = ""
    opponent_level: int = 0
    opponent_health: int = 0
    opponent_health_max: int = 0
    room_vnum: int = 0
    
    @property
    def hp_percent(self) -> float:
        if self.health_max <= 0:
            return 100.0
        return (self.health / self.health_max) * 100.0
    
    @property
    def mana_percent(self) -> float:
        if self.mana_max <= 0:
            return 100.0
        return (self.mana / self.mana_max) * 100.0
    
    @property
    def move_percent(self) -> float:
        if self.movement_max <= 0:
            return 100.0
        return (self.movement / self.movement_max) * 100.0
    
    @property
    def opponent_hp_percent(self) -> float:
        if self.opponent_health_max <= 0:
            return 100.0
        return (self.opponent_health / self.opponent_health_max) * 100.0


@dataclass
class RoomInfo:
    """Room information from MSDP."""
    name: str = ""
    vnum: int = 0
    exits: list[str] = field(default_factory=list)
    area: str = ""


class MSDPParser:
    """
    Parses MSDP subnegotiation data.
    
    MSDP format:
    - VAR (1) followed by variable name
    - VAL (2) followed by value
    - Values can be strings, tables (3/4), or arrays (5/6)
    """
    
    def __init__(self):
        self.variables: dict[str, Any] = {}
        self.stats = CharacterStats()
        self.room = RoomInfo()
        self._update_callbacks: list[Callable[[str, Any], None]] = []
        
    def add_update_callback(self, callback: Callable[[str, Any], None]) -> None:
        """Add callback to be called when a variable is updated."""
        self._update_callbacks.append(callback)
    
    def parse(self, data: bytes) -> dict[str, Any]:
        """
        Parse MSDP subnegotiation data.
        
        Args:
            data: Raw MSDP data (after option byte)
            
        Returns:
            Dict of variable name -> value pairs
        """
        result: dict[str, Any] = {}
        
        i = 0
        while i < len(data):
            if data[i] == MSDPCode.VAR:
                # Find variable name
                i += 1
                name_end = self._find_code(data, i)
                if name_end == -1:
                    break
                    
                var_name = data[i:name_end].decode('utf-8', errors='replace')
                i = name_end
                
                # Expect VAL next
                if i < len(data) and data[i] == MSDPCode.VAL:
                    i += 1
                    value, new_i = self._parse_value(data, i)
                    i = new_i
                    
                    result[var_name] = value
                    self._update_variable(var_name, value)
            else:
                i += 1
                
        return result
    
    def _find_code(self, data: bytes, start: int) -> int:
        """Find the next MSDP code byte."""
        for i in range(start, len(data)):
            if data[i] in (MSDPCode.VAR, MSDPCode.VAL, 
                          MSDPCode.TABLE_OPEN, MSDPCode.TABLE_CLOSE,
                          MSDPCode.ARRAY_OPEN, MSDPCode.ARRAY_CLOSE):
                return i
        return len(data)
    
    def _parse_value(self, data: bytes, start: int) -> tuple[Any, int]:
        """Parse a value starting at position."""
        if start >= len(data):
            return "", start
            
        # Check for table
        if data[start] == MSDPCode.TABLE_OPEN:
            return self._parse_table(data, start + 1)
            
        # Check for array
        if data[start] == MSDPCode.ARRAY_OPEN:
            return self._parse_array(data, start + 1)
            
        # Simple string value
        end = self._find_code(data, start)
        value = data[start:end].decode('utf-8', errors='replace')
        return value, end
    
    def _parse_table(self, data: bytes, start: int) -> tuple[dict, int]:
        """Parse a table value."""
        result: dict[str, Any] = {}
        i = start
        
        while i < len(data):
            if data[i] == MSDPCode.TABLE_CLOSE:
                return result, i + 1
                
            if data[i] == MSDPCode.VAR:
                i += 1
                name_end = self._find_code(data, i)
                key = data[i:name_end].decode('utf-8', errors='replace')
                i = name_end
                
                if i < len(data) and data[i] == MSDPCode.VAL:
                    i += 1
                    value, i = self._parse_value(data, i)
                    result[key] = value
            else:
                i += 1
                
        return result, i
    
    def _parse_array(self, data: bytes, start: int) -> tuple[list, int]:
        """Parse an array value."""
        result: list[Any] = []
        i = start
        
        while i < len(data):
            if data[i] == MSDPCode.ARRAY_CLOSE:
                return result, i + 1
                
            if data[i] == MSDPCode.VAL:
                i += 1
                value, i = self._parse_value(data, i)
                result.append(value)
            else:
                i += 1
                
        return result, i
    
    def _update_variable(self, name: str, value: Any) -> None:
        """Update internal state based on variable."""
        self.variables[name] = value
        
        # Update stats
        try:
            if name == "HEALTH":
                self.stats.health = int(value)
            elif name == "HEALTH_MAX":
                self.stats.health_max = int(value)
            elif name == "MANA":
                self.stats.mana = int(value)
            elif name == "MANA_MAX":
                self.stats.mana_max = int(value)
            elif name == "MOVEMENT":
                self.stats.movement = int(value)
            elif name == "MOVEMENT_MAX":
                self.stats.movement_max = int(value)
            elif name == "LEVEL":
                self.stats.level = int(value)
            elif name == "EXPERIENCE":
                self.stats.experience = int(value)
            elif name == "EXPERIENCE_MAX":
                self.stats.experience_max = int(value)
            elif name == "ALIGNMENT":
                self.stats.alignment = int(value)
            elif name == "MONEY":
                self.stats.money = int(value)
            elif name == "IN_COMBAT":
                self.stats.in_combat = int(value) != 0
            elif name == "OPPONENT_NAME":
                self.stats.opponent_name = str(value)
            elif name == "OPPONENT_LEVEL":
                self.stats.opponent_level = int(value)
            elif name == "OPPONENT_HEALTH":
                self.stats.opponent_health = int(value)
            elif name == "OPPONENT_HEALTH_MAX":
                self.stats.opponent_health_max = int(value)
            elif name == "ROOM_VNUM":
                self.stats.room_vnum = int(value)
            elif name == "ROOM":
                if isinstance(value, dict):
                    self.room.name = value.get("NAME", "")
                    self.room.vnum = int(value.get("VNUM", 0))
                    self.room.area = value.get("AREA", "")
            elif name == "ROOM_EXITS":
                if isinstance(value, list):
                    self.room.exits = [str(e) for e in value]
                elif isinstance(value, str):
                    self.room.exits = value.split()
        except (ValueError, TypeError) as e:
            logger.debug(f"Could not parse {name}={value}: {e}")
            
        # Notify callbacks
        for callback in self._update_callbacks:
            try:
                callback(name, value)
            except Exception as e:
                logger.error(f"MSDP callback error: {e}")
        
        logger.debug(f"MSDP {name} = {value}")


class GMCPParser:
    """
    Parser for GMCP (MSDP over GMCP).
    
    GMCP uses JSON format instead of MSDP binary format.
    """
    
    def __init__(self, msdp_parser: Optional[MSDPParser] = None):
        self.msdp = msdp_parser or MSDPParser()
        
    def parse(self, data: bytes) -> Optional[tuple[str, Any]]:
        """
        Parse GMCP message.
        
        Args:
            data: Raw GMCP data (after option byte)
            
        Returns:
            Tuple of (package.message, data) or None
        """
        import json
        
        try:
            decoded = data.decode('utf-8', errors='replace')
            
            # GMCP format: "Package.Message {json data}" or just "Package.Message"
            space_idx = decoded.find(' ')
            if space_idx == -1:
                return (decoded, None)
                
            package = decoded[:space_idx]
            json_str = decoded[space_idx + 1:]
            
            try:
                json_data = json.loads(json_str)
            except json.JSONDecodeError:
                json_data = json_str
                
            logger.debug(f"GMCP {package}: {json_data}")
            return (package, json_data)
            
        except Exception as e:
            logger.error(f"GMCP parse error: {e}")
            return None
