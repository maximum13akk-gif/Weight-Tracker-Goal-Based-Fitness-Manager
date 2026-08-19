# WeightTracker.cs
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (C# Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 * Requires: .NET 6.0+
 */

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.Json;
using System.Text.Json.Serialization;

class WeightTracker
{
    // ─── Colors ────────────────────────────────────────────────────────────

    private static readonly string Reset = "\u001B[0m";
    private static readonly string Bright = "\u001B[1m";
    private static readonly string Dim = "\u001B[2m";
    private static readonly string Red = "\u001B[31m";
    private static readonly string Green = "\u001B[32m";
    private static readonly string Yellow = "\u001B[33m";
    private static readonly string Blue = "\u001B[34m";
    private static readonly string Magenta = "\u001B[35m";
    private static readonly string Cyan = "\u001B[36m";

    private static string C(string text, string color) => color + text + Reset;

    // ─── Data Classes ──────────────────────────────────────────────────────

    public class Entry
    {
        [JsonPropertyName("date")]
        public string Date { get; set; } = "";
        [JsonPropertyName("weight")]
        public double Weight { get; set; }
        [JsonPropertyName("notes")]
        public string Notes { get; set; } = "";
    }

    public class Data
    {
        [JsonPropertyName("goalWeight")]
        public double GoalWeight { get; set; } = 75.0;
        [JsonPropertyName("goalDeadline")]
        public string GoalDeadline { get; set; } = "";
        [JsonPropertyName("unit")]
        public string Unit { get; set; } = "kg";
        [JsonPropertyName("height")]
        public double Height { get; set; } = 175.0;
        [JsonPropertyName("entries")]
        public List<Entry> Entries { get; set; } = new();
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private const double DefaultGoal = 75.0;
    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.UserProfile),
        ".weight_tracker"
    );
    private static readonly string DataFile = Path.Combine(DataDir, "data.json");

    // ─── Weight Tracker ────────────────────────────────────────────────────

    private readonly Data data = new();
    private readonly Random random = new();

    public WeightTracker()
    {
        Directory.CreateDirectory(DataDir);
        Load();
        if (string.IsNullOrEmpty(data.GoalDeadline))
            data.GoalDeadline = DefaultDeadline();
    }

    private void Load()
    {
        if (!File.Exists(DataFile)) return;
        try
        {
            string json = File.ReadAllText(DataFile);
            var loaded = JsonSerializer.Deserialize<Data>(json);
            if (loaded != null)
            {
                data.GoalWeight = loaded.GoalWeight > 0 ? loaded.GoalWeight : DefaultGoal;
                data.GoalDeadline = loaded.GoalDeadline ?? DefaultDeadline();
                data.Unit = !string.IsNullOrEmpty(loaded.Unit) ? loaded.Unit : "kg";
                data.Height = loaded.Height > 0 ? loaded.Height : 175.0;
                data.Entries = loaded.Entries ?? new List<Entry>();
            }
        }
        catch { /* ignore */ }
    }

    private void Save()
    {
        string json = JsonSerializer.Serialize(data, new JsonSerializerOptions { WriteIndented = true });
        File.WriteAllText(DataFile, json);
    }

    private static string DefaultDeadline() => DateTime.Now.AddDays(90).ToString("yyyy-MM-dd");

    private string Today() => DateTime.Now.ToString("yyyy-MM-dd");

    private Entry GetTodayEntry()
    {
        string today = Today();
        return data.Entries.FirstOrDefault(e => e.Date == today);
    }

    private double? GetTodayWeight()
    {
        var e = GetTodayEntry();
        return e != null ? e.Weight : (double?)null;
    }

    private List<Entry> GetAllEntriesSorted()
    {
        return data.Entries.OrderBy(e => e.Date).ToList();
    }

    private List<double> GetWeights()
    {
        return GetAllEntriesSorted().Select(e => e.Weight).ToList();
    }

    private string ProgressBar(double current, double goal, int width = 20)
    {
        if (goal <= 0) return "⚠️  Goal not set";
        var weights = GetWeights();
        if (!weights.Any()) return "No data";
        double start = weights.First();
        double ratio;
        if (current >= start)
            ratio = Math.Max(0, Math.Min(1, (current - goal) / (start - goal)));
        else
            ratio = Math.Max(0, Math.Min(1, (start - current) / (start - goal)));
        int filled = (int)(ratio * width);
        string bar = new string('█', filled) + new string('░', width - filled);
        return $"[{bar}] {ratio * 100.0:F1}%";
    }

    private string Ask(string prompt)
    {
        Console.Write(prompt);
        return Console.ReadLine()?.Trim() ?? "";
    }

    private double AskDouble(string prompt)
    {
        while (true)
        {
            if (double.TryParse(Ask(prompt), out double val)) return val;
            Console.WriteLine(C("❌ Please enter a number.", Red));
        }
    }

    private bool AskConfirm(string prompt)
    {
        string ans = Ask(prompt + " (yes/no): ").ToLower();
        return ans == "yes";
    }

    // ─── Core Actions ─────────────────────────────────────────────────────

    private void AddEntry(double weight, string notes)
    {
        if (weight <= 0)
        {
            Console.WriteLine(C("❌ Weight must be positive!", Red));
            return;
        }
        string today = Today();
        var existing = GetTodayEntry();
        if (existing != null)
        {
            existing.Weight = weight;
            existing.Notes = notes;
        }
        else
        {
            data.Entries.Add(new Entry { Date = today, Weight = weight, Notes = notes });
        }
        Save();
        Console.WriteLine(C($"✅ Weight logged: {weight:F1} {data.Unit}", Green));
    }

    private void ShowToday()
    {
        var weight = GetTodayWeight();
        if (weight == null)
        {
            Console.WriteLine(C("No weight logged today.", Yellow));
            return;
        }
        double diff = weight.Value - data.GoalWeight;
        string status = diff > 0 ? "above" : "below";
        string color = diff > 0 ? Red : Green;
        Console.WriteLine("\n" + C(new string('═', 50), Dim));
        Console.WriteLine(C("⚖️ TODAY'S WEIGHT", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Dim));
        Console.WriteLine($"  Weight: {C($"{weight:F1} {data.Unit}", color)}");
        Console.WriteLine($"  Goal:   {data.GoalWeight:F1} {data.Unit}");
        Console.WriteLine($"  You are {C($"{Math.Abs(diff):F1} {data.Unit} {status}", color)} your goal");
        Console.WriteLine($"  Progress: {ProgressBar(weight.Value, data.GoalWeight)}");
        Console.WriteLine(C(new string('═', 50), Dim));
    }

    private void ShowChart(int days = 30)
    {
        var entries = GetAllEntriesSorted();
        if (!entries.Any())
        {
            Console.WriteLine(C("No data to chart.", Yellow));
            return;
        }
        if (entries.Count > days)
            entries = entries.Skip(entries.Count - days).ToList();
        var weights = entries.Select(e => e.Weight).ToList();
        var dates = entries.Select(e => e.Date[5..10]).ToList();
        double minW = weights.Min();
        double maxW = weights.Max();
        double range = maxW - minW;
        if (range == 0)
        {
            Console.WriteLine(C("Weight is constant. No variation to chart.", Yellow));
            return;
        }
        int height = 10;
        int chartWidth = 40;
        var norm = weights.Select(w => (int)((w - minW) / range * (height - 1))).ToList();
        var lines = new List<string>();
        for (int row = height - 1; row >= 0; row--)
        {
            var line = new char[norm.Count];
            for (int i = 0; i < norm.Count; i++)
            {
                if (norm[i] >= row)
                {
                    if (i > 0 && norm[i - 1] >= row)
                        line[i] = '─';
                    else
                        line[i] = '┌';
                }
                else
                {
                    line[i] = ' ';
                }
            }
            lines.Add(new string(line));
        }
        int step = Math.Max(1, dates.Count / 8);
        var xAxis = new System.Text.StringBuilder(" ");
        int lastPos = 0;
        for (int i = 0; i < dates.Count; i += step)
        {
            if (i >= dates.Count) break;
            string label = dates[i];
            int pos = i;
            if (pos > lastPos) xAxis.Append(' ', pos - lastPos);
            xAxis.Append(label);
            lastPos = pos;
        }
        if (lastPos < dates.Count - 1)
            xAxis.Append(' ', dates.Count - 1 - lastPos);
        Console.WriteLine($"\n{C($"📈 Weight Chart (last {entries.Count} days)", Bright + Cyan)}");
        foreach (var line in lines) Console.WriteLine(line);
        Console.WriteLine(xAxis.ToString());
        Console.WriteLine($"Min: {minW:F1} {data.Unit}  Max: {maxW:F1} {data.Unit}");
    }

    private void ShowStats()
    {
        if (!data.Entries.Any())
        {
            Console.WriteLine(C("📭 No data yet. Start tracking!", Yellow));
            return;
        }
        var weights = GetWeights();
        if (!weights.Any())
        {
            Console.WriteLine(C("No weight data available.", Yellow));
            return;
        }
        double total = weights.Sum();
        double avg = total / weights.Count;
        double minW = weights.Min();
        double maxW = weights.Max();
        double last = weights.Last();
        double first = weights.First();
        double change = last - first;
        string trend = change > 0 ? "up" : "down";
        double heightM = data.Height / 100.0;
        double bmi = heightM > 0 ? last / (heightM * heightM) : 0;
        string bmiCat = BmiCategory(bmi);
        Console.WriteLine("\n📊 STATISTICS");
        Console.WriteLine(C(new string('─', 30), Dim));
        Console.WriteLine($"  Total Entries:  {weights.Count}");
        Console.WriteLine($"  Current Weight: {C($"{last:F1} {data.Unit}", Green)}");
        Console.WriteLine($"  Goal Weight:    {data.GoalWeight:F1} {data.Unit}");
        Console.WriteLine($"  Average:        {avg:F1} {data.Unit}");
        Console.WriteLine($"  Minimum:        {minW:F1} {data.Unit}");
        Console.WriteLine($"  Maximum:        {maxW:F1} {data.Unit}");
        string sign = change > 0 ? "+" : "";
        Console.WriteLine($"  Change:         {sign}{change:F1} {data.Unit} ({trend})");
        Console.WriteLine($"  BMI:            {bmi:F1} ({bmiCat})");
    }

    private static string BmiCategory(double bmi)
    {
        if (bmi < 18.5) return "Underweight";
        if (bmi < 25) return "Normal";
        if (bmi < 30) return "Overweight";
        return "Obese";
    }

    private void SetGoal(double weight, string deadline)
    {
        if (weight <= 0)
        {
            Console.WriteLine(C("❌ Goal weight must be positive!", Red));
            return;
        }
        data.GoalWeight = weight;
        if (!string.IsNullOrEmpty(deadline))
        {
            if (!DateTime.TryParseExact(deadline, "yyyy-MM-dd", null, System.Globalization.DateTimeStyles.None, out _))
            {
                Console.WriteLine(C("⚠️  Invalid date format. Use YYYY-MM-DD. Keeping current deadline.", Yellow));
                return;
            }
            data.GoalDeadline = deadline;
        }
        else
        {
            data.GoalDeadline = DefaultDeadline();
        }
        Save();
        Console.WriteLine(C($"✅ Goal set to {weight:F1} {data.Unit} by {data.GoalDeadline}", Green));
    }

    private void SetHeight(double height)
    {
        if (height <= 0)
        {
            Console.WriteLine(C("❌ Height must be positive!", Red));
            return;
        }
        data.Height = height;
        Save();
        Console.WriteLine(C($"✅ Height set to {height:F1} cm", Green));
    }

    private void SetUnit(string unit)
    {
        if (unit != "kg" && unit != "lbs")
        {
            Console.WriteLine(C("❌ Unit must be 'kg' or 'lbs'", Red));
            return;
        }
        data.Unit = unit;
        Save();
        Console.WriteLine(C($"✅ Unit set to {unit}", Green));
    }

    private void ClearData()
    {
        if (!AskConfirm("⚠️  Delete ALL data? This cannot be undone!")) return;
        data.Entries.Clear();
        data.GoalWeight = DefaultGoal;
        data.GoalDeadline = DefaultDeadline();
        Save();
        Console.WriteLine(C("🗑️  All data cleared.", Yellow));
    }

    // ─── Menu ─────────────────────────────────────────────────────────────

    private void ShowMenu()
    {
        var todayWeight = GetTodayWeight();
        string weightStr = todayWeight != null ? $"{todayWeight:F1}" : "—";
        string progress = ProgressBar(todayWeight ?? 0, data.GoalWeight);
        Console.WriteLine("\n" + C(new string('═', 50), Cyan));
        Console.WriteLine(C("⚖️ WEIGHT TRACKER", Bright + Cyan));
        Console.WriteLine(C(new string('═', 50), Cyan));
        Console.WriteLine($"  Today: {weightStr} {data.Unit} / {data.GoalWeight:F1} {data.Unit}");
        Console.WriteLine($"  Goal deadline: {data.GoalDeadline}");
        Console.WriteLine($"  Progress: {progress}");
        Console.WriteLine(C(new string('─', 50), Dim));
        Console.WriteLine("  1. ⚖️ Log weight today");
        Console.WriteLine("  2. 📊 Today's progress");
        Console.WriteLine("  3. 📈 Show weight chart");
        Console.WriteLine("  4. 📊 Statistics");
        Console.WriteLine("  5. 🎯 Set goal weight & deadline");
        Console.WriteLine("  6. 📏 Set height");
        Console.WriteLine("  7. 🔄 Set unit (kg/lbs)");
        Console.WriteLine("  8. 🗑️  Clear all data");
        Console.WriteLine("  0. 🚪 Exit");
        Console.WriteLine(C(new string('═', 50), Cyan));
    }

    public void Run()
    {
        Console.Clear();
        Console.WriteLine(C("\n⚖️ Weight Tracker – Goal‑Based Fitness Manager", Bright + Cyan));
        Console.WriteLine(C("Track your weight, reach your goals!", Dim));

        while (true)
        {
            ShowMenu();
            string choice = Ask("Your choice: ");
            switch (choice)
            {
                case "1":
                    double weight = AskDouble($"Weight ({data.Unit}): ");
                    string notes = Ask("Notes (optional): ");
                    AddEntry(weight, notes);
                    break;
                case "2":
                    ShowToday();
                    break;
                case "3":
                    ShowChart(30);
                    break;
                case "4":
                    ShowStats();
                    break;
                case "5":
                    double goal = AskDouble($"Goal weight ({data.Unit}): ");
                    string deadline = Ask("Deadline (YYYY-MM-DD) [leave empty for default]: ");
                    SetGoal(goal, deadline);
                    break;
                case "6":
                    double height = AskDouble("Height (cm): ");
                    SetHeight(height);
                    break;
                case "7":
                    string unit = Ask("Unit (kg/lbs): ");
                    SetUnit(unit.ToLower());
                    break;
                case "8":
                    ClearData();
                    break;
                case "0":
                    Console.WriteLine(C("👋 Stay fit! Goodbye!", Cyan));
                    return;
                default:
                    Console.WriteLine(C("❌ Invalid choice.", Red));
                    break;
            }
            if (choice != "0")
            {
                Console.Write("\nPress Enter to continue...");
                Console.ReadLine();
            }
        }
    }

    public static void Main()
    {
        try
        {
            new WeightTracker().Run();
        }
        catch (Exception ex)
        {
            Console.WriteLine(C($"❌ Unexpected error: {ex.Message}", Red));
            Environment.Exit(1);
        }
    }
}
