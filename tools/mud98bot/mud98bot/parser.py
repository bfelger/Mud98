"""
Text parser for Mud98Bot.

Parses game output to extract structured information.
"""

import re
import logging
from typing import Optional, NamedTuple
from dataclasses import dataclass, field
from enum import Enum, auto

logger = logging.getLogger(__name__)


class DamageTier(Enum):
    """Damage severity levels."""
    MISS = auto()
    SCRATCH = auto()
    LIGHT = auto()
    MODERATE = auto()
    HEAVY = auto()
    DEVASTATING = auto()
    LETHAL = auto()


# Damage verb to tier mapping (ROM-style)
DAMAGE_VERBS = {
    # Misses
    "miss": DamageTier.MISS,
    "misses": DamageTier.MISS,
    
    # Light damage
    "scratch": DamageTier.SCRATCH,
    "scratches": DamageTier.SCRATCH,
    "graze": DamageTier.SCRATCH,
    "grazes": DamageTier.SCRATCH,
    
    # Moderate
    "hit": DamageTier.LIGHT,
    "hits": DamageTier.LIGHT,
    "injure": DamageTier.MODERATE,
    "injures": DamageTier.MODERATE,
    "wound": DamageTier.MODERATE,
    "wounds": DamageTier.MODERATE,
    
    # Heavy
    "maul": DamageTier.HEAVY,
    "mauls": DamageTier.HEAVY,
    "decimate": DamageTier.HEAVY,
    "decimates": DamageTier.HEAVY,
    "devastate": DamageTier.DEVASTATING,
    "devastates": DamageTier.DEVASTATING,
    
    # Lethal
    "maim": DamageTier.LETHAL,
    "maims": DamageTier.LETHAL,
    "mutilate": DamageTier.LETHAL,
    "mutilates": DamageTier.LETHAL,
    "eviscerate": DamageTier.LETHAL,
    "eviscerates": DamageTier.LETHAL,
    "dismember": DamageTier.LETHAL,
    "dismembers": DamageTier.LETHAL,
}


@dataclass
class PromptInfo:
    """Parsed prompt information."""
    hp: int = 0
    hp_max: int = 0
    mana: int = 0
    mana_max: int = 0
    move: int = 0
    move_max: int = 0
    raw: str = ""


@dataclass
class RoomParsed:
    """Parsed room information from 'look' output."""
    name: str = ""
    description: str = ""
    exits: list[str] = field(default_factory=list)
    items: list[str] = field(default_factory=list)
    mobiles: list[str] = field(default_factory=list)


@dataclass
class CombatHit:
    """Single combat hit/miss."""
    attacker: str = ""
    target: str = ""
    verb: str = ""
    damage_tier: DamageTier = DamageTier.MISS
    is_player_attack: bool = False


@dataclass
class ItemInfo:
    """Basic item information."""
    name: str = ""
    keywords: list[str] = field(default_factory=list)
    level: int = 0
    item_type: str = ""
    wear_location: str = ""


# ============================================================================
# BOT Protocol Dataclasses (PLR_BOT structured output)
# ============================================================================

@dataclass
class BotRoom:
    """Parsed [BOT:ROOM|...] data."""
    vnum: int = 0
    flags: list[str] = field(default_factory=list)
    sector: str = ""


@dataclass
class BotExit:
    """Parsed [BOT:EXIT|...] data."""
    direction: str = ""
    vnum: int = 0
    flags: list[str] = field(default_factory=list)


@dataclass 
class BotMob:
    """Parsed [BOT:MOB|...] data."""
    name: str = ""
    vnum: int = 0
    level: int = 0
    flags: list[str] = field(default_factory=list)
    hp_percent: int = 100
    alignment: int = 0


@dataclass
class BotObj:
    """Parsed [BOT:OBJ|...] data."""
    name: str = ""
    vnum: int = 0
    item_type: str = ""
    flags: list[str] = field(default_factory=list)
    wear_flags: list[str] = field(default_factory=list)


@dataclass
class BotRoomData:
    """Complete room data from BOT protocol."""
    room: Optional[BotRoom] = None
    exits: list[BotExit] = field(default_factory=list)
    mobs: list[BotMob] = field(default_factory=list)
    objects: list[BotObj] = field(default_factory=list)


class TextParser:
    """
    Parses MUD text output into structured data.
    
    Uses regex patterns to identify and extract game information.
    """
    
    # Prompt patterns
    PROMPT_PATTERNS = [
        # <100hp 50m 200mv>
        re.compile(r'<(\d+)hp\s+(\d+)m\s+(\d+)mv>'),
        # <100/150hp 50/100m 200/250mv>
        re.compile(r'<(\d+)/(\d+)hp\s+(\d+)/(\d+)m\s+(\d+)/(\d+)mv>'),
        # [100hp 50m 200mv]
        re.compile(r'\[(\d+)hp\s+(\d+)m\s+(\d+)mv\]'),
    ]
    
    # Exit patterns
    EXIT_PATTERN = re.compile(r'\[Exits?:\s*([^\]]+)\]', re.IGNORECASE)
    OBVIOUS_EXITS = re.compile(r'Obvious exits?:\s*(.+)', re.IGNORECASE)
    
    # Combat patterns
    COMBAT_HIT = re.compile(
        r"(?:Your|(\w+(?:'s)?)) (\w+) (miss(?:es)?|\w+) (?:you|the )?(\w+)",
        re.IGNORECASE
    )
    KILL_MESSAGE = re.compile(r'(\w+) is DEAD!', re.IGNORECASE)
    XP_GAIN = re.compile(r'You (?:gain|receive) (\d+) (?:experience|exp)', re.IGNORECASE)
    
    # Loot patterns
    GOLD_PICKUP = re.compile(r'You get (\d+) gold', re.IGNORECASE)
    ITEM_PICKUP = re.compile(r'You get (.+?) from', re.IGNORECASE)
    
    # Inventory patterns
    CARRYING = re.compile(r'You are carrying:', re.IGNORECASE)
    
    # BOT protocol patterns (PLR_BOT structured output)
    BOT_ROOM = re.compile(r'\[BOT:ROOM\|([^\]]+)\]')
    BOT_EXIT = re.compile(r'\[BOT:EXIT\|([^\]]+)\]')
    BOT_MOB = re.compile(r'\[BOT:MOB\|([^\]]+)\]')
    BOT_OBJ = re.compile(r'\[BOT:OBJ\|([^\]]+)\]')
    
    # Condition patterns (for mob health)
    CONDITIONS = [
        ("excellent condition", 100),
        ("few scratches", 90),
        ("small wounds", 75),
        ("quite a few wounds", 55),
        ("nasty wounds", 35),
        ("bleeding freely", 20),
        ("leaking guts", 10),
        ("almost dead", 5),
    ]
    
    def parse_prompt(self, text: str) -> Optional[PromptInfo]:
        """Extract prompt information from text."""
        for pattern in self.PROMPT_PATTERNS:
            match = pattern.search(text)
            if match:
                groups = match.groups()
                info = PromptInfo(raw=match.group(0))
                
                if len(groups) == 3:
                    # hp m mv format
                    info.hp = int(groups[0])
                    info.mana = int(groups[1])
                    info.move = int(groups[2])
                elif len(groups) == 6:
                    # hp/max m/max mv/max format
                    info.hp = int(groups[0])
                    info.hp_max = int(groups[1])
                    info.mana = int(groups[2])
                    info.mana_max = int(groups[3])
                    info.move = int(groups[4])
                    info.move_max = int(groups[5])
                    
                return info
        return None
    
    def parse_exits(self, text: str) -> list[str]:
        """Extract available exits from room text."""
        exits = []
        
        # Try [Exits: n s e w] format
        match = self.EXIT_PATTERN.search(text)
        if match:
            exit_str = match.group(1).strip()
            if exit_str.lower() == 'none':
                return []
            # Split on whitespace
            exits = exit_str.split()
            
        # Try "Obvious exits:" format
        if not exits:
            match = self.OBVIOUS_EXITS.search(text)
            if match:
                exit_str = match.group(1).strip()
                # Parse "north south east west" or "n s e w"
                exits = exit_str.replace('.', '').split()
                
        # Normalize exit names
        exit_map = {
            'n': 'north', 's': 'south', 'e': 'east', 'w': 'west',
            'u': 'up', 'd': 'down'
        }
        normalized = []
        for e in exits:
            e = e.lower().strip()
            if e in exit_map:
                normalized.append(exit_map[e])
            elif e in exit_map.values():
                normalized.append(e)
                
        return normalized
    
    def parse_combat_line(self, line: str) -> Optional[CombatHit]:
        """Parse a single line of combat output."""
        # "Your slash EVISCERATES the cityguard!"
        # "The cityguard's punch scratches you."
        
        line_lower = line.lower()
        
        for verb, tier in DAMAGE_VERBS.items():
            if f" {verb} " in line_lower or f" {verb}." in line_lower:
                hit = CombatHit()
                hit.verb = verb
                hit.damage_tier = tier
                
                if line_lower.startswith("your "):
                    hit.is_player_attack = True
                    hit.attacker = "you"
                    # Extract target
                    parts = line.split(verb)
                    if len(parts) > 1:
                        target = parts[1].strip().rstrip('!.')
                        hit.target = target.replace("the ", "")
                else:
                    hit.is_player_attack = False
                    # Extract attacker
                    if "'s " in line:
                        hit.attacker = line.split("'s ")[0].replace("The ", "")
                    hit.target = "you"
                    
                return hit
                
        return None
    
    def parse_kill(self, text: str) -> Optional[str]:
        """Check for kill message, return victim name."""
        match = self.KILL_MESSAGE.search(text)
        if match:
            return match.group(1)
        return None
    
    def parse_xp_gain(self, text: str) -> int:
        """Extract XP gain from text."""
        match = self.XP_GAIN.search(text)
        if match:
            return int(match.group(1))
        return 0
    
    def parse_mob_condition(self, text: str) -> Optional[int]:
        """
        Parse mob condition from 'consider' or combat output.
        
        Returns estimated HP percentage.
        """
        text_lower = text.lower()
        for condition, hp_percent in self.CONDITIONS:
            if condition in text_lower:
                return hp_percent
        return None
    
    def find_attackable_mobs(self, room_text: str) -> list[str]:
        """
        Try to identify attackable mobs from room description.
        
        This is heuristic - looks for lines that might be mob descriptions.
        """
        mobs = []
        lines = room_text.split('\n')
        
        for line in lines:
            line = line.strip()
            if not line:
                continue
                
            # Skip player-looking lines
            if line.startswith('(') or '[' in line:
                continue
                
            # Skip obvious non-mob lines
            skip_words = ['exit', 'obvious', 'you see', 'is here']
            if any(w in line.lower() for w in skip_words):
                continue
                
            # Mob descriptions often end with "is here" or "stands here"
            if 'is here' in line.lower() or 'stands here' in line.lower():
                # Extract keyword (usually first word or word after article)
                words = line.split()
                for word in words:
                    word = word.lower().strip('.,!')
                    if word not in ('a', 'an', 'the', 'is', 'here', 'stands'):
                        mobs.append(word)
                        break
                        
        return mobs
    
    def is_error_message(self, text: str) -> bool:
        """Check if text contains an error message."""
        error_patterns = [
            "you can't",
            "you cannot",
            "you don't have",
            "you aren't",
            "you are not",
            "nothing happens",
            "you failed",
            "you don't see",
            "no one by that name",
            "they aren't here",
            "you are too tired",
            "you don't have enough",
        ]
        
        text_lower = text.lower()
        return any(p in text_lower for p in error_patterns)
    
    def is_combat_active(self, text: str) -> bool:
        """Check if combat appears to be active."""
        combat_indicators = [
            " hits you",
            " misses you",
            "your ",
            " is DEAD",
            " dodges your",
            " parries your",
        ]
        
        text_lower = text.lower()
        return any(i in text_lower for i in combat_indicators)

    # ========================================================================
    # BOT Protocol Parsing (PLR_BOT structured output)
    # ========================================================================
    
    def _parse_bot_fields(self, data: str) -> dict[str, str]:
        """Parse pipe-separated key=value fields."""
        fields = {}
        for part in data.split('|'):
            if '=' in part:
                key, value = part.split('=', 1)
                fields[key.strip()] = value.strip()
        return fields
    
    def _parse_flags(self, flags_str: str) -> list[str]:
        """Parse space-separated flags, handling (none)."""
        if flags_str == '(none)' or not flags_str:
            return []
        return flags_str.split()
    
    def parse_bot_room(self, text: str) -> Optional[BotRoom]:
        """Parse [BOT:ROOM|...] line."""
        match = self.BOT_ROOM.search(text)
        if not match:
            return None
            
        fields = self._parse_bot_fields(match.group(1))
        room = BotRoom()
        room.vnum = int(fields.get('vnum', 0))
        room.flags = self._parse_flags(fields.get('flags', ''))
        room.sector = fields.get('sector', '')
        return room
    
    def parse_bot_exit(self, text: str) -> Optional[BotExit]:
        """Parse [BOT:EXIT|...] line."""
        match = self.BOT_EXIT.search(text)
        if not match:
            return None
            
        fields = self._parse_bot_fields(match.group(1))
        exit_ = BotExit()
        exit_.direction = fields.get('dir', '')
        exit_.vnum = int(fields.get('vnum', 0))
        exit_.flags = self._parse_flags(fields.get('flags', ''))
        return exit_
    
    def parse_bot_mob(self, text: str) -> Optional[BotMob]:
        """Parse [BOT:MOB|...] line."""
        match = self.BOT_MOB.search(text)
        if not match:
            return None
            
        fields = self._parse_bot_fields(match.group(1))
        mob = BotMob()
        mob.name = fields.get('name', '')
        mob.vnum = int(fields.get('vnum', 0))
        mob.level = int(fields.get('level', 0))
        mob.flags = self._parse_flags(fields.get('flags', ''))
        # Parse hp=100% -> 100
        hp_str = fields.get('hp', '100%').rstrip('%')
        mob.hp_percent = int(hp_str) if hp_str else 100
        mob.alignment = int(fields.get('align', 0))
        return mob
    
    def parse_bot_obj(self, text: str) -> Optional[BotObj]:
        """Parse [BOT:OBJ|...] line."""
        match = self.BOT_OBJ.search(text)
        if not match:
            return None
            
        fields = self._parse_bot_fields(match.group(1))
        obj = BotObj()
        obj.name = fields.get('name', '')
        obj.vnum = int(fields.get('vnum', 0))
        obj.item_type = fields.get('type', '')
        obj.flags = self._parse_flags(fields.get('flags', ''))
        obj.wear_flags = self._parse_flags(fields.get('wear', ''))
        return obj
    
    def parse_bot_data(self, text: str) -> BotRoomData:
        """
        Parse all BOT protocol lines from text.
        
        Returns BotRoomData with room, exits, mobs, and objects populated.
        Use this after 'look' command when PLR_BOT is enabled.
        """
        data = BotRoomData()
        
        for line in text.split('\n'):
            if '[BOT:ROOM|' in line:
                data.room = self.parse_bot_room(line)
            elif '[BOT:EXIT|' in line:
                exit_ = self.parse_bot_exit(line)
                if exit_:
                    data.exits.append(exit_)
            elif '[BOT:MOB|' in line:
                mob = self.parse_bot_mob(line)
                if mob:
                    data.mobs.append(mob)
            elif '[BOT:OBJ|' in line:
                obj = self.parse_bot_obj(line)
                if obj:
                    data.objects.append(obj)
        
        return data
    
    def has_bot_data(self, text: str) -> bool:
        """Check if text contains BOT protocol data."""
        return '[BOT:' in text
