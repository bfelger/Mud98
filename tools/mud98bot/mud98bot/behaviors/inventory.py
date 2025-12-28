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
    SHOP_ROOM, INTERMEDIATE_ROOM, CORRIDOR_ROOM,
)

if TYPE_CHECKING:
    from ..bot import Bot

logger = logging.getLogger(__name__)


class LootBehavior(Behavior):
    """
    Loot corpses after combat and intelligently equip better items.
    
    Workflow:
    1. Get all items from corpse
    2. For each looted item, compare to equipped gear
    3. Wear items that are better or fill empty slots
    4. Drop items that are worse (to avoid inventory clutter)
    5. Sacrifice the corpse for gold
    """
    
    priority = PRIORITY_LOOT
    name = "Loot"
    tick_delay = 0.5
    
    # Compare command output patterns
    COMPARE_BETTER = "looks better than"
    COMPARE_WORSE = "looks worse than"
    COMPARE_SAME = "look about the same"
    COMPARE_NOTHING = "aren't wearing anything comparable"
    COMPARE_CANT = "can't compare"
    
    # States for the loot workflow
    STATE_LOOT = 0
    STATE_WAIT_LOOT = 1
    STATE_COMPARE = 2
    STATE_WAIT_COMPARE = 3
    STATE_SACRIFICE = 4
    STATE_DONE = 5
    
    def __init__(self):
        super().__init__()
        self._state = self.STATE_LOOT
        self._looted_items: list[str] = []
        self._current_item_index = 0
        self._cooldown_until = 0.0
        self._wait_ticks = 0
    
    def can_start(self, ctx: BehaviorContext) -> bool:
        """Start when there's a corpse and not in combat."""
        import time
        if time.time() < self._cooldown_until:
            return False
        if ctx.in_combat:
            return False
        return ctx.has_corpse
    
    def start(self, bot: 'Bot', ctx: BehaviorContext) -> None:
        super().start(bot, ctx)
        self._state = self.STATE_LOOT
        self._looted_items = []
        self._current_item_index = 0
        self._wait_ticks = 0
        logger.info(f"[{bot.bot_id}] Looting corpse...")
    
    def _parse_looted_items(self, text: str) -> list[str]:
        """Parse 'You get X from the corpse' messages to extract item names."""
        items = []
        pattern = re.compile(r"You get (.+?) from (?:the )?corpse", re.IGNORECASE)
        for match in pattern.finditer(text):
            item_name = match.group(1).strip()
            # Skip gold/coins
            if 'gold' in item_name.lower() or 'coin' in item_name.lower():
                continue
            # Skip items in parentheses (usually coins like "(6cp)")
            if item_name.startswith('(') and item_name.endswith(')'):
                continue
            # Extract keyword (last word is usually the primary keyword)
            words = item_name.split()
            if words:
                keyword = words[-1].rstrip('.')
                if keyword not in ('a', 'an', 'the', 'some'):
                    items.append(keyword)
        return items
    
    def _parse_compare_result(self, text: str) -> str:
        """Parse compare command output to determine action."""
        if self.COMPARE_NOTHING in text:
            return "wear"  # Nothing comparable equipped, wear it
        elif self.COMPARE_BETTER in text:
            return "wear"  # New item is better
        elif self.COMPARE_WORSE in text:
            return "drop"  # New item is worse
        elif self.COMPARE_SAME in text:
            return "keep"  # About the same, just keep in inventory
        elif self.COMPARE_CANT in text:
            return "keep"  # Can't compare (different types), keep for now
        else:
            return "unknown"
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        import time
        
        if self._state == self.STATE_LOOT:
            bot.send_command("get all corpse")
            self._state = self.STATE_WAIT_LOOT
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        elif self._state == self.STATE_WAIT_LOOT:
            self._wait_ticks += 1
            if self._wait_ticks >= 2:
                self._looted_items = self._parse_looted_items(ctx.last_text)
                if self._looted_items:
                    logger.info(f"[{bot.bot_id}] Looted items: {self._looted_items}")
                    self._state = self.STATE_COMPARE
                    self._current_item_index = 0
                else:
                    self._state = self.STATE_SACRIFICE
            return BehaviorResult.CONTINUE
        
        elif self._state == self.STATE_COMPARE:
            if self._current_item_index >= len(self._looted_items):
                self._state = self.STATE_SACRIFICE
                return BehaviorResult.CONTINUE
            
            item = self._looted_items[self._current_item_index]
            bot.send_command(f"compare {item}")
            self._state = self.STATE_WAIT_COMPARE
            self._wait_ticks = 0
            return BehaviorResult.CONTINUE
        
        elif self._state == self.STATE_WAIT_COMPARE:
            self._wait_ticks += 1
            if self._wait_ticks >= 2:
                item = self._looted_items[self._current_item_index]
                action = self._parse_compare_result(ctx.last_text)
                
                if action == "wear":
                    logger.info(f"[{bot.bot_id}] Equipping better item: {item}")
                    bot.send_command(f"wear {item}")
                elif action == "drop":
                    logger.info(f"[{bot.bot_id}] Dropping worse item: {item}")
                    bot.send_command(f"drop {item}")
                elif action == "keep":
                    logger.debug(f"[{bot.bot_id}] Keeping comparable item: {item}")
                elif action == "unknown":
                    logger.debug(f"[{bot.bot_id}] Unknown compare result for {item}, trying to wear")
                    bot.send_command(f"wear {item}")
                
                self._current_item_index += 1
                self._state = self.STATE_COMPARE
            return BehaviorResult.CONTINUE
        
        elif self._state == self.STATE_SACRIFICE:
            bot.send_command("sacrifice corpse")
            self._state = self.STATE_DONE
            return BehaviorResult.CONTINUE
        
        elif self._state == self.STATE_DONE:
            # Refresh room data
            bot.send_command("look")
            
            # Clear the text buffer after processing loot
            if hasattr(bot, '_behavior_engine'):
                bot._behavior_engine.clear_text_buffer()
            
            # Set cooldown to avoid re-triggering on stale data
            self._cooldown_until = time.time() + 5.0
            logger.info(f"[{bot.bot_id}] Looting complete")
            return BehaviorResult.COMPLETED
        
        return BehaviorResult.CONTINUE


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
    Navigate to shop, buy food/drink/lantern, then return.
    
    This is an emergent, transient behavior - only activates when the bot
    detects it is hungry or thirsty and has enough money to buy supplies.
    Also supports proactive shopping after patrol circuit completion.
    
    Shop is in room 3718 (south from 3717, which is down from 3712).
    Sells soup (VNUM 3151), water skin (VNUM 3138), and lantern.
    
    The lantern is needed to see in dark rooms like 3720 where the
    "big creature" with the key to MUD school is located.
    """
    
    priority = PRIORITY_BUY_SUPPLIES
    name = "BuySupplies"
    tick_delay = 0.5
    
    # State machine states
    STATE_GO_TO_SHOP = 'go_to_shop'
    STATE_BUY_FOOD = 'buy_food'
    STATE_BUY_DRINK = 'buy_drink'
    STATE_BUY_LANTERN = 'buy_lantern'
    STATE_EAT = 'eat'
    STATE_DRINK = 'drink'
    STATE_RETURN = 'return'
    
    # Costs (approximate)
    SOUP_COST = 10
    WATER_COST = 10
    LANTERN_COST = 20
    
    def __init__(self, buy_lantern: bool = True):
        super().__init__()
        self.buy_lantern = buy_lantern
        self._state = self.STATE_GO_TO_SHOP
        self._wait_ticks = 0
        self._bought_food = False
        self._bought_drink = False
        self._bought_lantern = False
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
        self._bought_lantern = False
        self._start_room = ctx.room_vnum
        self._is_proactive = ctx.should_proactive_shop and not ctx.is_hungry and not ctx.is_thirsty
        
        # Clear the proactive shopping flag
        if ctx.should_proactive_shop and hasattr(bot, '_behavior_engine') and bot._behavior_engine:
            bot._behavior_engine._should_proactive_shop = False
        
        needs = []
        if self._is_proactive:
            needs.append("food (proactive)")
            needs.append("drink (proactive)")
            if self.buy_lantern:
                needs.append("lantern (proactive)")
        else:
            if ctx.is_hungry:
                needs.append("food")
            if ctx.is_thirsty:
                needs.append("drink")
            if self.buy_lantern:
                needs.append("lantern")
        logger.info(f"[{bot.bot_id}] Going to shop to buy: {', '.join(needs)} (money: {ctx.money})")
    
    def tick(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        if self._state == self.STATE_GO_TO_SHOP:
            return self._navigate_to_shop(bot, ctx)
        elif self._state == self.STATE_BUY_FOOD:
            return self._buy_food(bot, ctx)
        elif self._state == self.STATE_BUY_DRINK:
            return self._buy_drink(bot, ctx)
        elif self._state == self.STATE_BUY_LANTERN:
            return self._buy_lantern(bot, ctx)
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
            # Next: buy lantern if needed, otherwise eat/drink or return
            if self.buy_lantern and not self._bought_lantern:
                self._state = self.STATE_BUY_LANTERN
            elif self._bought_food and not self._is_proactive:
                self._state = self.STATE_EAT
            elif not self._is_proactive:
                self._state = self.STATE_DRINK
            else:
                # Proactive: skip consuming
                self._state = self.STATE_RETURN
        return BehaviorResult.CONTINUE
    
    def _buy_lantern(self, bot: 'Bot', ctx: BehaviorContext) -> BehaviorResult:
        """Buy lantern at the shop for dark rooms."""
        if not self._bought_lantern:
            bot.send_command('buy lantern')
            bot.send_command('wear lantern')  # Equip it immediately
            self._bought_lantern = True
            self._wait_ticks = 0
            logger.info(f"[{bot.bot_id}] Bought and equipped lantern for dark rooms")
            return BehaviorResult.CONTINUE
        
        self._wait_ticks += 1
        if self._wait_ticks >= 2:
            if self._bought_food and not self._is_proactive:
                self._state = self.STATE_EAT
            elif self._bought_drink and not self._is_proactive:
                self._state = self.STATE_DRINK
            else:
                # Proactive: skip consuming, go to return
                logger.info(f"[{bot.bot_id}] Lantern bought, transitioning to RETURN state (proactive={self._is_proactive})")
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
        """Return to patrol area or navigate to dark creature room."""
        logger.debug(f"[{bot.bot_id}] _return_to_start: room={ctx.room_vnum}, proactive={self._is_proactive}, bought_lantern={self._bought_lantern}")
        
        # After proactive shopping with lantern, go to corridor (3719) for dark creature fight
        if self._is_proactive and self._bought_lantern:
            if ctx.room_vnum == CORRIDOR_ROOM:
                logger.info(f"[{bot.bot_id}] Finished proactive shopping, now at corridor (3719) for dark creature")
                # Set the flag to trigger FightDarkCreatureBehavior
                if hasattr(bot, '_behavior_engine') and bot._behavior_engine:
                    bot._behavior_engine._should_fight_dark_creature = True
                return BehaviorResult.COMPLETED
            elif ctx.room_vnum == SHOP_ROOM:
                bot.send_command('north')
            elif ctx.room_vnum == INTERMEDIATE_ROOM:
                # The east door to corridor may be closed - try opening first
                bot.send_command('open east')
                bot.send_command('east')  # Go east to corridor (3719), not up to cage (3712)
            else:
                # Somewhere unexpected, try to get back on route
                bot.send_command('recall')
            return BehaviorResult.CONTINUE
        
        # Normal shopping: return to cage area
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
