"""
Metrics collection for Mud98Bot stress testing.

Tracks connection stats, latency, throughput, and game events.
"""

import logging
import time
import threading
from dataclasses import dataclass, field
from typing import Optional
from collections import deque

logger = logging.getLogger(__name__)


@dataclass
class BotMetrics:
    """Metrics for a single bot instance."""
    bot_id: str = ""
    
    # Connection stats
    connected: bool = False
    connect_time: float = 0.0
    disconnect_time: float = 0.0
    connection_attempts: int = 0
    connection_failures: int = 0
    
    # Command stats
    commands_sent: int = 0
    responses_received: int = 0
    
    # Latency tracking (rolling window)
    _latency_samples: deque = field(default_factory=lambda: deque(maxlen=100))
    _last_command_time: float = 0.0
    
    # Throughput
    bytes_sent: int = 0
    bytes_received: int = 0
    
    # Game stats
    kills: int = 0
    deaths: int = 0
    xp_gained: int = 0
    rooms_visited: int = 0
    gold_earned: int = 0
    
    # State
    current_behavior: str = ""
    current_room_vnum: int = 0
    hp_percent: float = 100.0
    
    # Errors
    parse_errors: int = 0
    timeout_errors: int = 0
    
    def record_command_sent(self, bytes_count: int = 0) -> None:
        """Record a command being sent."""
        self.commands_sent += 1
        self.bytes_sent += bytes_count
        self._last_command_time = time.time()
    
    def record_response_received(self, bytes_count: int = 0) -> None:
        """Record a response being received."""
        self.responses_received += 1
        self.bytes_received += bytes_count
        
        # Calculate latency if we have a pending command
        if self._last_command_time > 0:
            latency = (time.time() - self._last_command_time) * 1000  # ms
            self._latency_samples.append(latency)
            self._last_command_time = 0
    
    def record_kill(self, xp: int = 0) -> None:
        """Record a mob kill."""
        self.kills += 1
        self.xp_gained += xp
    
    def record_death(self) -> None:
        """Record bot death."""
        self.deaths += 1
    
    @property
    def latency_avg_ms(self) -> float:
        """Average latency in milliseconds."""
        if not self._latency_samples:
            return 0.0
        return sum(self._latency_samples) / len(self._latency_samples)
    
    @property
    def latency_p99_ms(self) -> float:
        """99th percentile latency in milliseconds."""
        if not self._latency_samples:
            return 0.0
        sorted_samples = sorted(self._latency_samples)
        idx = int(len(sorted_samples) * 0.99)
        return sorted_samples[min(idx, len(sorted_samples) - 1)]
    
    @property
    def uptime_seconds(self) -> float:
        """Time connected in seconds."""
        if not self.connected or self.connect_time == 0:
            return 0.0
        end_time = self.disconnect_time if self.disconnect_time > 0 else time.time()
        return end_time - self.connect_time


@dataclass
class AggregateMetrics:
    """Aggregated metrics across all bots."""
    start_time: float = 0.0
    end_time: float = 0.0
    
    # Totals
    total_bots: int = 0
    bots_connected: int = 0
    bots_playing: int = 0
    
    total_commands: int = 0
    total_responses: int = 0
    total_bytes_sent: int = 0
    total_bytes_received: int = 0
    
    total_kills: int = 0
    total_deaths: int = 0
    total_xp: int = 0
    
    total_connection_attempts: int = 0
    total_connection_failures: int = 0
    total_parse_errors: int = 0
    total_timeout_errors: int = 0
    
    # Averages
    avg_latency_ms: float = 0.0
    p99_latency_ms: float = 0.0
    
    @property
    def duration_seconds(self) -> float:
        """Total test duration."""
        if self.start_time == 0:
            return 0.0
        end = self.end_time if self.end_time > 0 else time.time()
        return end - self.start_time
    
    @property
    def commands_per_second(self) -> float:
        """Average commands per second across all bots."""
        duration = self.duration_seconds
        if duration <= 0:
            return 0.0
        return self.total_commands / duration
    
    @property
    def kills_per_minute(self) -> float:
        """Kills per minute across all bots."""
        duration = self.duration_seconds
        if duration <= 0:
            return 0.0
        return (self.total_kills / duration) * 60
    
    @property
    def connection_success_rate(self) -> float:
        """Percentage of successful connections."""
        if self.total_connection_attempts == 0:
            return 100.0
        successes = self.total_connection_attempts - self.total_connection_failures
        return (successes / self.total_connection_attempts) * 100


class MetricsCollector:
    """
    Collects and aggregates metrics from multiple bot instances.
    
    Thread-safe for concurrent bot updates.
    """
    
    def __init__(self):
        self._lock = threading.Lock()
        self._bot_metrics: dict[str, BotMetrics] = {}
        self._start_time: float = 0.0
        self._end_time: float = 0.0
    
    def start(self) -> None:
        """Mark the start of metrics collection."""
        with self._lock:
            self._start_time = time.time()
            self._end_time = 0.0
    
    def stop(self) -> None:
        """Mark the end of metrics collection."""
        with self._lock:
            self._end_time = time.time()
    
    def register_bot(self, bot_id: str) -> BotMetrics:
        """Register a bot and return its metrics object."""
        with self._lock:
            if bot_id not in self._bot_metrics:
                self._bot_metrics[bot_id] = BotMetrics(bot_id=bot_id)
            return self._bot_metrics[bot_id]
    
    def get_bot_metrics(self, bot_id: str) -> Optional[BotMetrics]:
        """Get metrics for a specific bot."""
        with self._lock:
            return self._bot_metrics.get(bot_id)
    
    def get_aggregate(self) -> AggregateMetrics:
        """Calculate aggregate metrics across all bots."""
        with self._lock:
            agg = AggregateMetrics(
                start_time=self._start_time,
                end_time=self._end_time,
                total_bots=len(self._bot_metrics)
            )
            
            all_latencies = []
            
            for metrics in self._bot_metrics.values():
                # Connection counts
                if metrics.connected:
                    agg.bots_connected += 1
                if metrics.current_behavior:
                    agg.bots_playing += 1
                
                # Totals
                agg.total_commands += metrics.commands_sent
                agg.total_responses += metrics.responses_received
                agg.total_bytes_sent += metrics.bytes_sent
                agg.total_bytes_received += metrics.bytes_received
                agg.total_kills += metrics.kills
                agg.total_deaths += metrics.deaths
                agg.total_xp += metrics.xp_gained
                agg.total_connection_attempts += metrics.connection_attempts
                agg.total_connection_failures += metrics.connection_failures
                agg.total_parse_errors += metrics.parse_errors
                agg.total_timeout_errors += metrics.timeout_errors
                
                # Collect latency samples
                all_latencies.extend(metrics._latency_samples)
            
            # Calculate aggregate latencies
            if all_latencies:
                agg.avg_latency_ms = sum(all_latencies) / len(all_latencies)
                sorted_latencies = sorted(all_latencies)
                idx = int(len(sorted_latencies) * 0.99)
                agg.p99_latency_ms = sorted_latencies[min(idx, len(sorted_latencies) - 1)]
            
            return agg
    
    def print_summary(self) -> None:
        """Print a summary of current metrics."""
        agg = self.get_aggregate()
        
        print("\n" + "=" * 60)
        print("STRESS TEST METRICS SUMMARY")
        print("=" * 60)
        
        print(f"\nDuration: {agg.duration_seconds:.1f} seconds")
        print(f"Bots: {agg.bots_connected}/{agg.total_bots} connected, "
              f"{agg.bots_playing} playing")
        
        print(f"\nConnections:")
        print(f"  Attempts: {agg.total_connection_attempts}")
        print(f"  Failures: {agg.total_connection_failures}")
        print(f"  Success Rate: {agg.connection_success_rate:.1f}%")
        
        print(f"\nThroughput:")
        print(f"  Commands Sent: {agg.total_commands}")
        print(f"  Commands/sec: {agg.commands_per_second:.1f}")
        print(f"  Bytes Sent: {agg.total_bytes_sent:,}")
        print(f"  Bytes Received: {agg.total_bytes_received:,}")
        
        print(f"\nLatency:")
        print(f"  Average: {agg.avg_latency_ms:.1f} ms")
        print(f"  P99: {agg.p99_latency_ms:.1f} ms")
        
        print(f"\nGame Stats:")
        print(f"  Kills: {agg.total_kills}")
        print(f"  Deaths: {agg.total_deaths}")
        print(f"  XP Gained: {agg.total_xp:,}")
        print(f"  Kills/min: {agg.kills_per_minute:.1f}")
        
        print(f"\nErrors:")
        print(f"  Parse Errors: {agg.total_parse_errors}")
        print(f"  Timeout Errors: {agg.total_timeout_errors}")
        
        print("=" * 60)
    
    def print_live_status(self) -> None:
        """Print a compact live status line."""
        agg = self.get_aggregate()
        
        print(f"\r[{agg.duration_seconds:6.1f}s] "
              f"Bots: {agg.bots_connected}/{agg.total_bots} | "
              f"Cmds: {agg.total_commands:5d} ({agg.commands_per_second:.1f}/s) | "
              f"Lat: {agg.avg_latency_ms:5.1f}ms | "
              f"Kills: {agg.total_kills} | "
              f"Deaths: {agg.total_deaths}",
              end='', flush=True)
    
    def to_dict(self) -> dict:
        """Export metrics as dictionary (for JSON output)."""
        agg = self.get_aggregate()
        return {
            "duration_seconds": agg.duration_seconds,
            "bots": {
                "total": agg.total_bots,
                "connected": agg.bots_connected,
                "playing": agg.bots_playing
            },
            "connections": {
                "attempts": agg.total_connection_attempts,
                "failures": agg.total_connection_failures,
                "success_rate": agg.connection_success_rate
            },
            "throughput": {
                "commands_sent": agg.total_commands,
                "commands_per_second": agg.commands_per_second,
                "bytes_sent": agg.total_bytes_sent,
                "bytes_received": agg.total_bytes_received
            },
            "latency": {
                "avg_ms": agg.avg_latency_ms,
                "p99_ms": agg.p99_latency_ms
            },
            "game": {
                "kills": agg.total_kills,
                "deaths": agg.total_deaths,
                "xp_gained": agg.total_xp,
                "kills_per_minute": agg.kills_per_minute
            },
            "errors": {
                "parse": agg.total_parse_errors,
                "timeout": agg.total_timeout_errors
            },
            "per_bot": {
                bot_id: {
                    "connected": m.connected,
                    "commands": m.commands_sent,
                    "kills": m.kills,
                    "hp_percent": m.hp_percent,
                    "behavior": m.current_behavior
                }
                for bot_id, m in self._bot_metrics.items()
            }
        }


# Global metrics collector instance
_global_collector: Optional[MetricsCollector] = None


def get_collector() -> MetricsCollector:
    """Get the global metrics collector."""
    global _global_collector
    if _global_collector is None:
        _global_collector = MetricsCollector()
    return _global_collector


def reset_collector() -> MetricsCollector:
    """Reset and return a fresh global metrics collector."""
    global _global_collector
    _global_collector = MetricsCollector()
    return _global_collector
