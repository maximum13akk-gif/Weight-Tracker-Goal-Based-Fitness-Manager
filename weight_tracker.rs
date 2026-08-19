# weight_tracker.rs
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (Rust Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 * Dependencies: serde, serde_json, chrono, colored
 */

use chrono::{Local, Duration};
use colored::*;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs;
use std::io::{self, Write, BufRead};
use std::path::PathBuf;

// ─── Types ──────────────────────────────────────────────────────────────────

#[derive(Debug, Serialize, Deserialize, Clone)]
struct Entry {
    date: String,
    weight: f64,
    notes: String,
}

#[derive(Debug, Serialize, Deserialize)]
struct Data {
    goal_weight: f64,
    goal_deadline: String,
    unit: String,
    height: f64,
    entries: Vec<Entry>,
}

// ─── Colors ──────────────────────────────────────────────────────────────────

fn c(text: &str, color: &str) -> String {
    match color {
        "green" => text.green().to_string(),
        "red" => text.red().to_string(),
        "yellow" => text.yellow().to_string(),
        "cyan" => text.cyan().to_string(),
        "bright" => text.bright().to_string(),
        "dim" => text.dimmed().to_string(),
        _ => text.to_string(),
    }
}

// ─── Config ──────────────────────────────────────────────────────────────────

const DEFAULT_GOAL: f64 = 75.0;
const DEFAULT_UNIT: &str = "kg";
const DEFAULT_HEIGHT: f64 = 175.0;

// ─── Data Manager ──────────────────────────────────────────────────────────

struct WeightTracker {
    goal_weight: f64,
    goal_deadline: String,
    unit: String,
    height: f64,
    entries: Vec<Entry>,
    file_path: PathBuf,
}

impl WeightTracker {
    fn new() -> Self {
        let home = std::env::var("HOME").or_else(|_| std::env::var("USERPROFILE")).unwrap_or_else(|_| ".".to_string());
        let dir = PathBuf::from(home).join(".weight_tracker");
        fs::create_dir_all(&dir).unwrap();
        let file_path = dir.join("data.json");
        let mut wt = WeightTracker {
            goal_weight: DEFAULT_GOAL,
            goal_deadline: default_deadline(),
            unit: DEFAULT_UNIT.to_string(),
            height: DEFAULT_HEIGHT,
            entries: Vec::new(),
            file_path,
        };
        wt.load();
        wt
    }

    fn load(&mut self) {
        if let Ok(raw) = fs::read_to_string(&self.file_path) {
            if let Ok(data) = serde_json::from_str::<Data>(&raw) {
                self.goal_weight = if data.goal_weight > 0.0 { data.goal_weight } else { DEFAULT_GOAL };
                self.goal_deadline = if data.goal_deadline.is_empty() { default_deadline() } else { data.goal_deadline };
                self.unit = if data.unit.is_empty() { DEFAULT_UNIT.to_string() } else { data.unit };
                self.height = if data.height > 0.0 { data.height } else { DEFAULT_HEIGHT };
                self.entries = data.entries;
                return;
            }
        }
        self.goal_weight = DEFAULT_GOAL;
        self.goal_deadline = default_deadline();
        self.unit = DEFAULT_UNIT.to_string();
        self.height = DEFAULT_HEIGHT;
        self.entries = Vec::new();
    }

    fn save(&self) {
        let data = Data {
            goal_weight: self.goal_weight,
            goal_deadline: self.goal_deadline.clone(),
            unit: self.unit.clone(),
            height: self.height,
            entries: self.entries.clone(),
        };
        let raw = serde_json::to_string_pretty(&data).unwrap();
        let _ = fs::write(&self.file_path, raw);
    }

    fn today(&self) -> String {
        Local::now().format("%Y-%m-%d").to_string()
    }

    fn get_today_entry(&mut self) -> Option<&mut Entry> {
        let today = self.today();
        self.entries.iter_mut().find(|e| e.date == today)
    }

    fn get_today_weight(&self) -> Option<f64> {
        let today = self.today();
        self.entries.iter().find(|e| e.date == today).map(|e| e.weight)
    }

    fn get_all_entries_sorted(&self) -> Vec<Entry> {
        let mut sorted = self.entries.clone();
        sorted.sort_by(|a, b| a.date.cmp(&b.date));
        sorted
    }

    fn get_weights(&self) -> Vec<f64> {
        self.get_all_entries_sorted().iter().map(|e| e.weight).collect()
    }

    fn progress_bar(&self, current: f64, goal: f64, width: usize) -> String {
        if goal <= 0.0 {
            return "⚠️  Goal not set".to_string();
        }
        let weights = self.get_weights();
        if weights.is_empty() {
            return "No data".to_string();
        }
        let start = weights[0];
        let ratio = if current >= start {
            ((current - goal) / (start - goal)).max(0.0).min(1.0)
        } else {
            ((start - current) / (start - goal)).max(0.0).min(1.0)
        };
        let filled = (ratio * width as f64) as usize;
        let bar = "█".repeat(filled) + &"░".repeat(width - filled);
        format!("[{}] {:.1}%", bar, ratio * 100.0)
    }

    fn ask(&self, prompt: &str) -> String {
        print!("{}", prompt);
        io::stdout().flush().unwrap();
        let mut line = String::new();
        io::stdin().read_line(&mut line).unwrap();
        line.trim().to_string()
    }

    fn ask_float(&self, prompt: &str) -> f64 {
        loop {
            let ans = self.ask(prompt);
            if let Ok(val) = ans.parse::<f64>() {
                return val;
            }
            println!("{}", c("❌ Please enter a number.", "red"));
        }
    }

    fn ask_confirm(&self, prompt: &str) -> bool {
        let ans = self.ask(&format!("{} (yes/no): ", prompt));
        ans.to_lowercase() == "yes"
    }

    // ─── Core Actions ──────────────────────────────────────────────────────

    fn add_entry(&mut self, weight: f64, notes: String) -> Result<(), Box<dyn std::error::Error>> {
        if weight <= 0.0 {
            return Err("Weight must be positive!".into());
        }
        let today = self.today();
        if let Some(entry) = self.get_today_entry() {
            entry.weight = weight;
            entry.notes = notes;
        } else {
            self.entries.push(Entry { date: today, weight, notes });
        }
        self.save();
        println!("{}", c(&format!("✅ Weight logged: {:.1} {}", weight, self.unit), "green"));
        Ok(())
    }

    fn show_today(&self) {
        if let Some(weight) = self.get_today_weight() {
            let diff = weight - self.goal_weight;
            let status = if diff > 0.0 { "above" } else { "below" };
            let color = if diff > 0.0 { "red" } else { "green" };
            println!("\n{}", "═".repeat(50).dimmed());
            println!("{}", c("⚖️ TODAY'S WEIGHT", "bright cyan"));
            println!("{}", "═".repeat(50).dimmed());
            println!("  Weight: {}", c(&format!("{:.1} {}", weight, self.unit), color));
            println!("  Goal:   {:.1} {}", self.goal_weight, self.unit);
            println!("  You are {} your goal", c(&format!("{:.1} {} {}", diff.abs(), self.unit, status), color));
            println!("  Progress: {}", self.progress_bar(weight, self.goal_weight, 20));
            println!("{}", "═".repeat(50).dimmed());
        } else {
            println!("{}", c("No weight logged today.", "yellow"));
        }
    }

    fn show_chart(&self, days: usize) {
        let mut entries = self.get_all_entries_sorted();
        if entries.is_empty() {
            println!("{}", c("No data to chart.", "yellow"));
            return;
        }
        if entries.len() > days {
            entries = entries.into_iter().skip(entries.len() - days).collect();
        }
        let weights: Vec<f64> = entries.iter().map(|e| e.weight).collect();
        let dates: Vec<String> = entries.iter().map(|e| e.date[5..10].to_string()).collect();
        let min_w = weights.iter().cloned().fold(f64::INFINITY, f64::min);
        let max_w = weights.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
        let range = max_w - min_w;
        if range == 0.0 {
            println!("{}", c("Weight is constant. No variation to chart.", "yellow"));
            return;
        }
        let height = 10;
        let chart_width = 40;
        let norm: Vec<usize> = weights.iter().map(|w| ((w - min_w) / range * (height - 1) as f64) as usize).collect();
        let mut lines = Vec::new();
        for row in (0..height).rev() {
            let mut line = String::new();
            for (i, &val) in norm.iter().enumerate() {
                if val >= row {
                    if i > 0 && norm[i-1] >= row {
                        line.push('─');
                    } else {
                        line.push('┌');
                    }
                } else {
                    line.push(' ');
                }
            }
            lines.push(line);
        }
        let step = (dates.len() / 8).max(1);
        let mut x_axis = " ".to_string();
        let mut last_pos = 0;
        for (i, label) in dates.iter().enumerate().step_by(step) {
            let pos = i;
            if pos > last_pos {
                x_axis.push_str(&" ".repeat(pos - last_pos));
            }
            x_axis.push_str(label);
            last_pos = pos;
        }
        if last_pos < dates.len() - 1 {
            x_axis.push_str(&" ".repeat(dates.len() - 1 - last_pos));
        }
        println!("\n{}", c(&format!("📈 Weight Chart (last {} days)", entries.len()), "bright cyan"));
        println!("{}", lines.join("\n"));
        println!("{}", x_axis);
        println!("Min: {:.1} {}  Max: {:.1} {}", min_w, self.unit, max_w, self.unit);
    }

    fn show_stats(&self) {
        if self.entries.is_empty() {
            println!("{}", c("📭 No data yet. Start tracking!", "yellow"));
            return;
        }
        let weights = self.get_weights();
        if weights.is_empty() {
            println!("{}", c("No weight data available.", "yellow"));
            return;
        }
        let total = weights.len();
        let sum: f64 = weights.iter().sum();
        let avg = sum / total as f64;
        let min_w = weights.iter().cloned().fold(f64::INFINITY, f64::min);
        let max_w = weights.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
        let last = weights[weights.len()-1];
        let first = weights[0];
        let change = last - first;
        let trend = if change > 0.0 { "up" } else { "down" };
        let height_m = self.height / 100.0;
        let bmi = if height_m > 0.0 { last / (height_m * height_m) } else { 0.0 };
        let bmi_cat = bmi_category(bmi);
        println!("\n📊 STATISTICS");
        println!("{}", "─".repeat(30).dimmed());
        println!("  Total Entries:  {}", total);
        println!("  Current Weight: {}", c(&format!("{:.1} {}", last, self.unit), "green"));
        println!("  Goal Weight:    {:.1} {}", self.goal_weight, self.unit);
        println!("  Average:        {:.1} {}", avg, self.unit);
        println!("  Minimum:        {:.1} {}", min_w, self.unit);
        println!("  Maximum:        {:.1} {}", max_w, self.unit);
        let sign = if change > 0.0 { "+" } else { "" };
        println!("  Change:         {}{:.1} {} ({})", sign, change, self.unit, trend);
        println!("  BMI:            {:.1} ({})", bmi, bmi_cat);
    }

    fn set_goal(&mut self, weight: f64, deadline: Option<String>) -> Result<(), Box<dyn std::error::Error>> {
        if weight <= 0.0 {
            return Err("Goal weight must be positive!".into());
        }
        self.goal_weight = weight;
        if let Some(d) = deadline {
            if chrono::NaiveDate::parse_from_str(&d, "%Y-%m-%d").is_err() {
                return Err("Invalid date format. Use YYYY-MM-DD".into());
            }
            self.goal_deadline = d;
        } else {
            self.goal_deadline = default_deadline();
        }
        self.save();
        println!("{}", c(&format!("✅ Goal set to {:.1} {} by {}", weight, self.unit, self.goal_deadline), "green"));
        Ok(())
    }

    fn set_height(&mut self, height: f64) -> Result<(), Box<dyn std::error::Error>> {
        if height <= 0.0 {
            return Err("Height must be positive!".into());
        }
        self.height = height;
        self.save();
        println!("{}", c(&format!("✅ Height set to {:.1} cm", height), "green"));
        Ok(())
    }

    fn set_unit(&mut self, unit: String) -> Result<(), Box<dyn std::error::Error>> {
        if unit != "kg" && unit != "lbs" {
            return Err("Unit must be 'kg' or 'lbs'".into());
        }
        self.unit = unit;
        self.save();
        println!("{}", c(&format!("✅ Unit set to {}", self.unit), "green"));
        Ok(())
    }

    fn clear_data(&mut self) {
        if !self.ask_confirm("⚠️  Delete ALL data? This cannot be undone!") {
            return;
        }
        self.entries = Vec::new();
        self.goal_weight = DEFAULT_GOAL;
        self.goal_deadline = default_deadline();
        self.save();
        println!("{}", c("🗑️  All data cleared.", "yellow"));
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    fn show_menu(&self) {
        let today_weight = self.get_today_weight();
        let weight_str = if let Some(w) = today_weight { format!("{:.1}", w) } else { "—".to_string() };
        let progress = self.progress_bar(today_weight.unwrap_or(0.0), self.goal_weight, 20);
        println!("\n{}", "═".repeat(50).cyan());
        println!("{}", c("⚖️ WEIGHT TRACKER", "bright cyan"));
        println!("{}", "═".repeat(50).cyan());
        println!("  Today: {} {} / {:.1} {}", weight_str, self.unit, self.goal_weight, self.unit);
        println!("  Goal deadline: {}", self.goal_deadline);
        println!("  Progress: {}", progress);
        println!("{}", "─".repeat(50).dimmed());
        println!("  1. ⚖️ Log weight today");
        println!("  2. 📊 Today's progress");
        println!("  3. 📈 Show weight chart");
        println!("  4. 📊 Statistics");
        println!("  5. 🎯 Set goal weight & deadline");
        println!("  6. 📏 Set height");
        println!("  7. 🔄 Set unit (kg/lbs)");
        println!("  8. 🗑️  Clear all data");
        println!("  0. 🚪 Exit");
        println!("{}", "═".repeat(50).cyan());
    }

    fn run(&mut self) {
        println!("{}", "\n⚖️ Weight Tracker – Goal‑Based Fitness Manager".bright().cyan());
        println!("{}", "Track your weight, reach your goals!".dimmed());

        loop {
            self.show_menu();
            let choice = self.ask("Your choice: ");
            match choice.as_str() {
                "1" => {
                    let weight = self.ask_float(&format!("Weight ({}): ", self.unit));
                    let notes = self.ask("Notes (optional): ");
                    if let Err(e) = self.add_entry(weight, notes) {
                        println!("{}", c(&format!("❌ {}", e), "red"));
                    }
                }
                "2" => self.show_today(),
                "3" => self.show_chart(30),
                "4" => self.show_stats(),
                "5" => {
                    let weight = self.ask_float(&format!("Goal weight ({}): ", self.unit));
                    let deadline = self.ask("Deadline (YYYY-MM-DD) [leave empty for default]: ");
                    if let Err(e) = self.set_goal(weight, if deadline.is_empty() { None } else { Some(deadline) }) {
                        println!("{}", c(&format!("❌ {}", e), "red"));
                    }
                }
                "6" => {
                    let height = self.ask_float("Height (cm): ");
                    if let Err(e) = self.set_height(height) {
                        println!("{}", c(&format!("❌ {}", e), "red"));
                    }
                }
                "7" => {
                    let unit = self.ask("Unit (kg/lbs): ");
                    if let Err(e) = self.set_unit(unit) {
                        println!("{}", c(&format!("❌ {}", e), "red"));
                    }
                }
                "8" => self.clear_data(),
                "0" => {
                    println!("{}", c("👋 Stay fit! Goodbye!", "cyan"));
                    return;
                }
                _ => println!("{}", c("❌ Invalid choice.", "red")),
            }
            if choice != "0" {
                print!("\nPress Enter to continue...");
                io::stdout().flush().unwrap();
                let mut _dummy = String::new();
                io::stdin().read_line(&mut _dummy).unwrap();
            }
        }
    }
}

fn default_deadline() -> String {
    (Local::now() + Duration::days(90)).format("%Y-%m-%d").to_string()
}

fn bmi_category(bmi: f64) -> String {
    if bmi < 18.5 { "Underweight".to_string() }
    else if bmi < 25.0 { "Normal".to_string() }
    else if bmi < 30.0 { "Overweight".to_string() }
    else { "Obese".to_string() }
}

// ─── Main ────────────────────────────────────────────────────────────────────

fn main() {
    let mut app = WeightTracker::new();
    app.run();
}
