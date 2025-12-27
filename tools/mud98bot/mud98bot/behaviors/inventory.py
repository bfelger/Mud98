"""
Inventory Behaviors - Looting, Shopping, Equipment.

This module contains behaviors for inventory management:
- LootBehavior: Loot corpses after combat, equip items
- ShopBehavior: Basic shopping (placeholder)
- BuySuppliesBehavior: Navigate to shop for food/water
"""

from __future__ import annotations

import logging
import re
from typing import TYPE_CHECKING, Optional

from .base import Behavior, BehaviorContext, BehaviorResult
from ..config import (
    PRIORITY_LOOT, PRIORITY_SHOP, PRIORITY_BUY_SUPPLIES,
    MIN_SHOPPING_MONEY, CENTRAL_ROOM, CAGE_ROOMS,
    SHOP_ROOM, INTERMEDIATE_ROOM,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class LootBehavior(Behavior):
    """
    Loot corpses after combat.
    
    After killing a mob, loot gold and items from the corpse.
    Can optionally auto-equip weapons and armor.
    """
    
    priority = PRIORITY_LOOT
    name = "Loot"
    tick_delay = 0.5
    
    # Patterns to match dropped/got items
    ITEM_PATTERNS = [
        re.compile(r'You get (.+) from the corpse'),
        re.compile(r'You get (.+) from corpse'),
        re.compile(r'You get (.+)\.'),
    ]
    
    # Item types to auto-equip
    EQUIP_SLOTS = {
        'sword': 'wield',
        'dagger': 'wield', 
        'mace': 'wield',
        'axe': 'wield',
        'staff': 'wield',
        'armor': 'wear',
        'helmet': 'wear',
        'shield': 'wear',
        'boots': 'wear',
        'gloves': 'wear',
        'cloak': 'wear',
        'ring': 'wear',
        'bracelet': 'wear',
        'amulet': 'wear',
        'belt': 'wear',
        'leggings': 'wear',
    }
    
    def __init__(self, auto_equip: bool = True, auto_get_gold: bool = True):
        super().__init__()
        self.auto_equip = auto_equip
        self.auto_get_gold = auto_get_gold
        self._looted = False
        self._equipped = False
        self._wait_ticks = 0
        self._pending_items: list[str] = []
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when there's a corpse and not in combat."""
        if ctx.in_combat:
            return False
        return ctx.has_corpse
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if ctx.in_combat:
            # Interrupted by combat
            return BehaviorResult.COMPLETED
        
        # Phase 1: Loot the corpse
        if not self._looted:
            bot.send_command("get all corpse")
            if self.auto_get_gold:
                bot.send_command("get gold corpse")
            self._looted = True
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        # Phase 2: Wait for loot output
        self._wait_ticks += 1
        if self._wait_ticks < 2:
            return BehaviorResult.CONTINUE
        
        # Phase 3: Parse items from text and equip
        if self.auto_equip and not self._equipped:
            items = self._parse_looted_items(ctx.last_text)
            for item in items:
                self._try_equip_item(bot, item)
            self._equipped = True
            return BehaviorResult.CONTINUE
        
        # Done looting
        self._looted = False
        self._equipped = False
        self._wait_ticks = 0
        return BehaviorResult.COMPLETED
    
    def _parse_looted_items(self, text: str) -> list[str]:
        """Parse item names from loot output."""
        items = []
        for pattern in self.ITEM_PATTERNS:
            matches = pattern.findall(text)
            items.extend(matches)
        return items
    
    def _try_equip_item(self, bot: 'Bot', item_name: str) -> None:
        """Try to equip an item if it's equipment."""
        item_lower = item_name.lower()
        for keyword, command in self.EQUIP_SLOTS.items():
            if keyword in item_lower:
                # Extract first word as keyword for command
                first_word = item_name.split()[0]
                logger.info(f"[{bot.bot_id}] Equipping: {first_word}")
                bot.send_command(f"{command} {first_word}")
                break


class ShopBehavior(Behavior):
    """
    Buy supplies from shops (placeholder).
    
    This is a basic placeholder. For full shopping functionality,
    see BuySuppliesBehavior which handles navigation.
    """
    
    priority = PRIORITY_SHOP
    name = "Shop"
    tick_delay = 0.5
    
    def __init__(self, buy_items: Optional[list[str]] = None):
        super().__init__()
        self.buy_items = buy_items or ['bread', 'water']
        self._at_shop = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Currently disabled - use BuySuppliesBehavior instead."""
        return False
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        return BehaviorResult.FAILED


class BuySuppliesBehavior(Behavior):
    """
    Navigate to shop, buy food/drink when hungry/thirsty, then return.
    
    This is an emergent, transient behavior - only activates when the bot
    detects it is hungry or thirsty and has enough money to buy supplies.
    Also supports proactive shopping after patrol circuit completion.
    
    Shop is in room 3718 (south from 3717, which is down from 3712).
    Sells soup (VNUM 3151) and water skin (VNUM 3138) from midgaard.are.
    """
    
    priority = PRIORITY_BUY_SUPPLIES
    name = "BuySupplies"
    tick_delay = 0.5
    
    # State machine states
    STATE_GO_TO_SHOP = 'go_to_shop'
    STATE_BUY_FOOD = 'buy_food'
    STATE_BUY_DRINK = 'buy_drink'
    STATE_EAT = 'eat'
    STATE_DRINK = 'drink'
    STATE_RETURN = 'return'
    
    # Costs (approximate)
    SOUP_COST = 10
    WATER_COST = 10
    
    def __init__(self):
        super().__init__()
        self._state = self.STATE_GO_TO_SHOP
        self._wait_ticks = 0
        self._bought_food = False
        self._bought_drink = False
        self._start_room: int = 0
        self._is_proactive = False
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start for reactive (hungry/thirsty) or proactive (circuit done) reasons."""
        if self._started:
            return True
        
        needs_reactive = ctx.is_hungry or ctx.is_thirsty
        needs_proactive = ctx.should_proactive_shop
        
        if not needs_reactive and not needs_proactive:
            return False
        
        if ctx.money < MIN_SHOPPING_MONEY:
            if needs_proactive:
                logger.debug(f"BuySuppliesBehavior: not enough money ({ctx.money} < {MIN_SHOPPING_MONEY})")
            return False
        
        if ctx.in_combat:
            return False
        
        # Only start from cage area or central room
        if ctx.room_vnum not in CAGE_ROOMS and ctx.room_vnum != CENTRAL_ROOM:
            if needs_proactive:
                logger.debug(f"BuySuppliesBehavior: not in cage area (room {ctx.room_vnum})")
            return False
        
        if needs_proactive:
            logger.info("BuySuppliesBehavior: proactive shopping approved!")
        return True
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._state = self.STATE_GO_TO_SHOP
        self._wait_ticks = 0
        self._bought_food = False
        self._bought_drink = False
        self._start_room = ctx.room_vnum
        self._is_proactive = ctx.should_proactive_shop and not ctx.is_hungry and not ctx.is_thirsty
        
        # Clear the proactive shopping flag
        if ctx.should_proactive_shop and hasattr(bot, '_behavior_engine') and bot._behavior_engine:
            bot._behavior_engine._should_proactive_shop = False
        
        needs = []
        if self._is_proactive:
            needs.append("food (proactive)")
            needs.append("drink (proactive)")
        else:
            if ctx.is_hungry:
                needs.append("food")
            if ctx.is_thirsty:
                needs.append("drink")
        logger.info(f"[{bot.bot_id}] Going to shop to buy: {', '.join(needs)} (money: {ctx.money})")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if self._state == self.STATE_GO_TO_SHOP:
            return self._navigate_to_shop(bot, ctx)
        elif self._state == self.STATE_BUY_FOOD:
            return self._buy_food(bot, ctx)
        elif self._state == self.STATE_BUY_DRINK:
            return self._buy_drink(bot, ctx)
        elif self._state == self.STATE_EAT:
            return self._eat(bot, ctx)
        elif self._state == self.STATE_DRINK:
            return self._drink(bot, ctx)
        elif self._state == self.STATE_RETURN:
            return self._return_to_start(bot, ctx)
        
        return BehaviorResult.FAILED
    
    def _navigate_to_shop(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Navigate from cage area to shop."""
        if ctx.room_vnum == SHOP_ROOM:
            logger.debug(f"[{bot.bot_id}] Arrived at shop")
            if self._is_proactive or ctx.is_hungry:
                self._state = self.STATE_BUY_FOOD
            elif ctx.is_thirsty:
                self._state = self.STATE_BUY_DRINK
            else:
                self._state = self.STATE_RETURN
            return BehaviorResult.CONTINUE
        
        # Navigate to shop
        if ctx.room_vnum in CAGE_ROOMS:
            exit_dirs = {3713: 'south', 3714: 'east', 3715: 'north', 3716: 'west'}
            direction = exit_dirs.get(ctx.room_vnum, 'south')
            bot.send_command(direction)
        elif ctx.room_vnum == CENTRAL_ROOM:
            bot.send_command('down')
        elif ctx.room_vnum == INTERMEDIATE_ROOM:
            bot.send_command('south')
        else:
            logger.warning(f"[{bot.bot_id}] Lost while shopping at room {ctx.room_vnum}")
            bot.send_command('recall')
            return BehaviorResult.FAILED
        
        return BehaviorResult.CONTINUE
    
    def _buy_food(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Buy soup at the shop."""
        if not self._bought_food:
            bot.send_command('buy soup')
            self._bought_food = True
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        self._wait_ticks += 1
        if self._wait_ticks >= 2:
            if self._is_proactive or ctx.is_thirsty:
                self._state = self.STATE_BUY_DRINK
            else:
                self._state = self.STATE_EAT
        return BehaviorResult.CONTINUE
    
    def _buy_drink(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Buy water skin at the shop."""
        if not self._bought_drink:
            bot.send_command('buy skin')
            self._bought_drink = True
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        self._wait_ticks += 1
        if self._wait_ticks >= 2:
            if self._bought_food and not self._is_proactive:
                self._state = self.STATE_EAT
            elif not self._is_proactive:
                self._state = self.STATE_DRINK
            else:
                # Proactive: skip consuming
                self._state = self.STATE_RETURN
        return BehaviorResult.CONTINUE
    
    def _eat(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Eat the soup."""
        bot.send_command('eat soup')
        self._wait_ticks = 0
        if self._bought_drink and not self._is_proactive:
            self._state = self.STATE_DRINK
        else:
            self._state = self.STATE_RETURN
        return BehaviorResult.CONTINUE
    
    def _drink(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Drink from the water skin."""
        bot.send_command('drink skin')
        self._wait_ticks += 1
        if self._wait_ticks >= 3:
            self._state = self.STATE_RETURN
        return BehaviorResult.CONTINUE
    
    def _return_to_start(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Return to the cage area."""
        if ctx.room_vnum == CENTRAL_ROOM or ctx.room_vnum in CAGE_ROOMS:
            if self._is_proactive:
                logger.info(f"[{bot.bot_id}] Finished proactive shopping, returned to room {ctx.room_vnum}")
            else:
                logger.info(f"[{bot.bot_id}] Finished shopping, returned to room {ctx.room_vnum}")
            return BehaviorResult.COMPLETED
        
        if ctx.room_vnum == SHOP_ROOM:
            bot.send_command('north')
        elif ctx.room_vnum == INTERMEDIATE_ROOM:
            bot.send_command('up')
        else:
            bot.send_command('recall')
        
        return BehaviorResult.CONTINUE
