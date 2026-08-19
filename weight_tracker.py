# weight_tracker.py
#!/usr/bin/env python3
"""
⚖️ Weight Tracker Goals – Fitness Manager (Python Edition)
Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
"""

import json
import os
import sys
import math
from datetime import datetime, timedelta
from pathlib import Path
from typing import List, Dict, Optional

try:
    from rich.console import Console
    from rich.table import Table
    from rich.panel import Panel
    from rich.prompt import Prompt, FloatPrompt, Confirm, IntPrompt
    from rich import box
    RICH_AVAILABLE = True
except ImportError:
    RICH_AVAILABLE = False
    print("⚠️  Install 'rich' for enhanced UI: pip install rich")


# ─── Colors ──────────────────────────────────────────────────────────────────

def c(text: str, color: str) -> str:
    colors = {
        "reset": "\033[0m", "bright": "\033[1m", "dim": "\033[2m",
        "red": "\033[31m", "green": "\033[32m", "yellow": "\033[33m",
        "blue": "\033[34m", "magenta": "\033[35m", "cyan": "\033[36m"
    }
    return f"{colors.get(color, '')}{text}{colors['reset']}"


# ─── Data Manager ──────────────────────────────────────────────────────────

class WeightTracker:
    DATA_DIR = Path.home() / ".weight_tracker"
    DATA_FILE = DATA_DIR / "data.json"
    DEFAULT_GOAL = 75.0
    DEFAULT_UNIT = "kg"

    def __init__(self):
        self.console = Console() if RICH_AVAILABLE else None
        self.data = self._load()
        self.goal_weight = self.data.get("goal_weight", self.DEFAULT_GOAL)
        self.goal_deadline = self.data.get("goal_deadline", (datetime.now() + timedelta(days=90)).strftime("%Y-%m-%d"))
        self.unit = self.data.get("unit", self.DEFAULT_UNIT)
        self.height = self.data.get("height", 175.0)  # cm
        self.entries: List[Dict] = self.data.get("entries", [])

    def _load(self) -> Dict:
        if self.DATA_FILE.exists():
            try:
                with open(self.DATA_FILE, 'r') as f:
                    return json.load(f)
            except Exception:
                return {}
        return {}

    def _save(self) -> None:
        self.DATA_DIR.mkdir(parents=True, exist_ok=True)
        with open(self.DATA_FILE, 'w') as f:
            json.dump({
                "goal_weight": self.goal_weight,
                "goal_deadline": self.goal_deadline,
                "unit": self.unit,
                "height": self.height,
                "entries": self.entries
            }, f, indent=2)

    def _today(self) -> str:
        return datetime.now().strftime("%Y-%m-%d")

    def _get_today_entry(self) -> Optional[Dict]:
        today = self._today()
        for e in self.entries:
            if e.get("date") == today:
                return e
        return None

    def _get_today_weight(self) -> Optional[float]:
        e = self._get_today_entry()
        return e.get("weight") if e else None

    def _get_all_entries_sorted(self) -> List[Dict]:
        return sorted(self.entries, key=lambda e: e["date"])

    def _get_weights(self) -> List[float]:
        return [e["weight"] for e in self._get_all_entries_sorted() if "weight" in e]

    def _progress_bar(self, current: float, goal: float, width: int = 20) -> str:
        if goal <= 0:
            return "⚠️  Goal not set"
        # For weight loss, progress is from max_weight to goal
        weights = self._get_weights()
        if not weights:
            return "No data"
        start = weights[0]
        if current >= start:
            # Gaining weight? Show based on current vs goal
            ratio = max(0, min(1, (current - goal) / (start - goal))) if start != goal else 1
        else:
            ratio = max(0, min(1, (start - current) / (start - goal))) if start != goal else 1
        filled = int(ratio * width)
        bar = "█" * filled + "░" * (width - filled)
        return f"[{bar}] {ratio*100:.1f}%"

    def add_entry(self, weight: float, notes: str = "") -> None:
        if weight <= 0:
            print(c("❌ Weight must be positive!", "red"))
            return
        today = self._today()
        entry = self._get_today_entry()
        if entry:
            entry["weight"] = weight
            entry["notes"] = notes
        else:
            self.entries.append({
                "date": today,
                "weight": weight,
                "notes": notes
            })
        self._save()
        print(c(f"✅ Weight logged: {weight:.1f} {self.unit}", "green"))

    def show_today(self) -> None:
        weight = self._get_today_weight()
        if weight is None:
            print(c("No weight logged today.", "yellow"))
            return
        goal = self.goal_weight
        diff = weight - goal
        status = "above" if diff > 0 else "below"
        color = "red" if diff > 0 else "green"
        if self.console:
            panel = Panel(
                f"[bold]⚖️ Today's Weight[/bold]\n"
                f"  Weight: {weight:.1f} {self.unit}\n"
                f"  Goal:   {goal:.1f} {self.unit}\n"
                f"  You are {abs(diff):.1f} {self.unit} {status} your goal\n"
                f"  Progress: {self._progress_bar(weight, goal)}",
                title="📊 Daily Progress",
                border_style="cyan"
            )
            self.console.print(panel)
        else:
            print("\n" + "="*50)
            print(c("⚖️ TODAY'S WEIGHT", "bright"))
            print("="*50)
            print(f"  Weight: {weight:.1f} {self.unit}")
            print(f"  Goal:   {goal:.1f} {self.unit}")
            print(f"  You are {abs(diff):.1f} {self.unit} {status} your goal")
            print(f"  Progress: {self._progress_bar(weight, goal)}")
            print("="*50)

    def show_chart(self, days: int = 30) -> None:
        entries = self._get_all_entries_sorted()
        if not entries:
            print(c("No data to chart.", "yellow"))
            return
        # Take last N days
        if len(entries) > days:
            entries = entries[-days:]
        weights = [e["weight"] for e in entries]
        dates = [e["date"][5:10] for e in entries]
        max_w = max(weights)
        min_w = min(weights)
        range_w = max_w - min_w
        if range_w == 0:
            print(c("Weight is constant. No variation to chart.", "yellow"))
            return
        height = 10
        chart_width = 40
        # Normalize
        norm = [int((w - min_w) / range_w * (height - 1)) for w in weights]
        lines = []
        for row in range(height - 1, -1, -1):
            line = ""
            for i, val in enumerate(norm):
                if val >= row:
                    if i > 0 and norm[i-1] >= row:
                        line += "─"
                    else:
                        line += "┌"
                else:
                    line += " "
            lines.append(line)
        # X-axis labels
        step = max(1, len(dates) // 8)
        x_axis = " "
        last_pos = 0
        for i in range(0, len(dates), step):
            label = dates[i]
            pos = i
            if pos > last_pos:
                x_axis += " " * (pos - last_pos)
            x_axis += label
            last_pos = pos
        if last_pos < len(dates) - 1:
            x_axis += " " * (len(dates) - 1 - last_pos)

        if self.console:
            self.console.print(f"\n[bold cyan]📈 Weight Chart (last {len(entries)} days)[/bold cyan]")
            self.console.print("\n".join(lines))
            self.console.print(x_axis)
            self.console.print(f"Min: {min_w:.1f} {self.unit}  Max: {max_w:.1f} {self.unit}")
        else:
            print(f"\n📈 Weight Chart (last {len(entries)} days)")
            print("\n".join(lines))
            print(x_axis)
            print(f"Min: {min_w:.1f} {self.unit}  Max: {max_w:.1f} {self.unit}")

    def show_stats(self) -> None:
        if not self.entries:
            print(c("📭 No data yet. Start tracking!", "yellow"))
            return
        weights = self._get_weights()
        if not weights:
            print(c("No weight data available.", "yellow"))
            return
        total = len(weights)
        avg = sum(weights) / total
        min_w = min(weights)
        max_w = max(weights)
        last = weights[-1]
        first = weights[0]
        change = last - first
        trend = "up" if change > 0 else "down"
        # BMI
        height_m = self.height / 100.0
        bmi = last / (height_m * height_m) if height_m > 0 else 0
        bmi_category = self._bmi_category(bmi)

        if self.console:
            table = Table(title="📊 Statistics", box=box.ROUNDED)
            table.add_column("Metric", style="cyan")
            table.add_column("Value", style="green")
            table.add_row("Total Entries", str(total))
            table.add_row("Current Weight", f"{last:.1f} {self.unit}")
            table.add_row("Goal Weight", f"{self.goal_weight:.1f} {self.unit}")
            table.add_row("Average", f"{avg:.1f} {self.unit}")
            table.add_row("Minimum", f"{min_w:.1f} {self.unit}")
            table.add_row("Maximum", f"{max_w:.1f} {self.unit}")
            table.add_row("Change", f"{change:+.1f} {self.unit} ({trend})")
            table.add_row("BMI", f"{bmi:.1f} ({bmi_category})")
            self.console.print(table)
        else:
            print("\n📊 STATISTICS")
            print(c("─"*30, "dim"))
            print(f"  Total Entries:  {total}")
            print(f"  Current Weight: {last:.1f} {self.unit}")
            print(f"  Goal Weight:    {self.goal_weight:.1f} {self.unit}")
            print(f"  Average:        {avg:.1f} {self.unit}")
            print(f"  Minimum:        {min_w:.1f} {self.unit}")
            print(f"  Maximum:        {max_w:.1f} {self.unit}")
            print(f"  Change:         {change:+.1f} {self.unit} ({trend})")
            print(f"  BMI:            {bmi:.1f} ({bmi_category})")

    def _bmi_category(self, bmi: float) -> str:
        if bmi < 18.5:
            return "Underweight"
        elif bmi < 25:
            return "Normal"
        elif bmi < 30:
            return "Overweight"
        else:
            return "Obese"

    def set_goal(self, weight: float, deadline: Optional[str] = None) -> None:
        if weight <= 0:
            print(c("❌ Goal weight must be positive!", "red"))
            return
        self.goal_weight = weight
        if deadline:
            try:
                datetime.strptime(deadline, "%Y-%m-%d")
                self.goal_deadline = deadline
            except ValueError:
                print(c("⚠️  Invalid date format. Use YYYY-MM-DD. Keeping current deadline.", "yellow"))
        else:
            self.goal_deadline = (datetime.now() + timedelta(days=90)).strftime("%Y-%m-%d")
        self._save()
        print(c(f"✅ Goal set to {weight:.1f} {self.unit} by {self.goal_deadline}", "green"))

    def set_height(self, height: float) -> None:
        if height <= 0:
            print(c("❌ Height must be positive!", "red"))
            return
        self.height = height
        self._save()
        print(c(f"✅ Height set to {height:.1f} cm", "green"))

    def set_unit(self, unit: str) -> None:
        if unit not in ["kg", "lbs"]:
            print(c("❌ Unit must be 'kg' or 'lbs'", "red"))
            return
        self.unit = unit
        self._save()
        print(c(f"✅ Unit set to {unit}", "green"))

    def clear_data(self) -> None:
        if self.console:
            if not Confirm.ask("⚠️  Delete ALL data? This cannot be undone!"):
                return
        else:
            if input("⚠️  Delete ALL data? (yes/no): ").strip().lower() != "yes":
                return
        self.entries = []
        self.goal_weight = self.DEFAULT_GOAL
        self.goal_deadline = (datetime.now() + timedelta(days=90)).strftime("%Y-%m-%d")
        self._save()
        print(c("🗑️  All data cleared.", "yellow"))

    # ─── Menu ──────────────────────────────────────────────────────────────

    def _show_menu(self) -> None:
        today_weight = self._get_today_weight()
        progress = self._progress_bar(today_weight if today_weight is not None else 0, self.goal_weight)
        if self.console:
            menu = f"""
[bold cyan]⚖️ Weight Tracker[/bold cyan]
  Today: {today_weight if today_weight is not None else '—'} {self.unit} / {self.goal_weight} {self.unit}
  Goal deadline: {self.goal_deadline}
  Progress: {progress}

  [1] ⚖️ Log weight today
  [2] 📊 Today's progress
  [3] 📈 Show weight chart
  [4] 📊 Statistics
  [5] 🎯 Set goal weight & deadline
  [6] 📏 Set height
  [7] 🔄 Set unit (kg/lbs)
  [8] 🗑️  Clear all data
  [0] 🚪 Exit
"""
            self.console.print(Panel(menu, border_style="blue"))
        else:
            print("\n" + "-"*50)
            print(f"⚖️ Today: {today_weight if today_weight is not None else '—'} {self.unit} / {self.goal_weight} {self.unit}")
            print(f"   Goal deadline: {self.goal_deadline}")
            print(f"   Progress: {progress}")
            print("-"*50)
            print("  1. ⚖️ Log weight today")
            print("  2. 📊 Today's progress")
            print("  3. 📈 Show weight chart")
            print("  4. 📊 Statistics")
            print("  5. 🎯 Set goal weight & deadline")
            print("  6. 📏 Set height")
            print("  7. 🔄 Set unit (kg/lbs)")
            print("  8. 🗑️  Clear all data")
            print("  0. 🚪 Exit")
            print("-"*50)

    def _get_choice(self) -> str:
        if self.console:
            return Prompt.ask("Your choice", choices=["0","1","2","3","4","5","6","7","8"])
        return input("Your choice: ").strip()

    def _get_weight(self) -> Optional[float]:
        if self.console:
            return FloatPrompt.ask(f"Weight ({self.unit})")
        try:
            return float(input(f"Weight ({self.unit}): ").strip())
        except ValueError:
            print(c("❌ Please enter a number.", "red"))
            return None

    def _get_notes(self) -> str:
        if self.console:
            return Prompt.ask("Notes (optional)", default="")
        return input("Notes (optional): ").strip()

    def _get_goal_weight(self) -> Optional[float]:
        if self.console:
            return FloatPrompt.ask(f"Goal weight ({self.unit})")
        try:
            return float(input(f"Goal weight ({self.unit}): ").strip())
        except ValueError:
            print(c("❌ Please enter a number.", "red"))
            return None

    def _get_deadline(self) -> str:
        if self.console:
            return Prompt.ask("Deadline (YYYY-MM-DD)", default=self.goal_deadline)
        return input(f"Deadline (YYYY-MM-DD) [current: {self.goal_deadline}]: ").strip() or self.goal_deadline

    def _get_height(self) -> Optional[float]:
        if self.console:
            return FloatPrompt.ask("Height (cm)")
        try:
            return float(input("Height (cm): ").strip())
        except ValueError:
            print(c("❌ Please enter a number.", "red"))
            return None

    def run(self) -> None:
        if self.console:
            self.console.print(Panel.fit("[bold cyan]⚖️ Weight Tracker – Goal‑Based Fitness Manager[/bold cyan]", border_style="cyan"))
        else:
            print(c("\n⚖️ Weight Tracker – Goal‑Based Fitness Manager", "bright"))
            print(c("Track your weight, reach your goals!", "dim"))

        while True:
            self._show_menu()
            choice = self._get_choice()

            if choice == "1":
                weight = self._get_weight()
                if weight is not None:
                    notes = self._get_notes()
                    self.add_entry(weight, notes)
            elif choice == "2":
                self.show_today()
            elif choice == "3":
                self.show_chart()
            elif choice == "4":
                self.show_stats()
            elif choice == "5":
                goal = self._get_goal_weight()
                if goal is not None:
                    deadline = self._get_deadline()
                    self.set_goal(goal, deadline)
            elif choice == "6":
                height = self._get_height()
                if height is not None:
                    self.set_height(height)
            elif choice == "7":
                unit = input("Unit (kg/lbs): ").strip().lower()
                self.set_unit(unit)
            elif choice == "8":
                self.clear_data()
            elif choice == "0":
                print(c("👋 Stay fit! Goodbye!", "cyan"))
                break
            else:
                print(c("❌ Invalid choice.", "red"))

            if choice != "0":
                if self.console:
                    self.console.print("\n[dim]Press Enter to continue...[/dim]")
                    input()
                else:
                    input("\nPress Enter to continue...")


def main():
    try:
        app = WeightTracker()
        app.run()
    except KeyboardInterrupt:
        print("\n👋 Goodbye!")
        sys.exit(0)
    except Exception as e:
        print(c(f"❌ Unexpected error: {e}", "red"))
        sys.exit(1)

if __name__ == "__main__":
    main()
