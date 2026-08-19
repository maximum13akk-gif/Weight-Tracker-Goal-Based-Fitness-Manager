# weight_tracker.go
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (Go Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 */

package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"strconv"
	"strings"
	"time"
)

// ─── Types ──────────────────────────────────────────────────────────────────

type Entry struct {
	Date   string  `json:"date"`
	Weight float64 `json:"weight"`
	Notes  string  `json:"notes"`
}

type Data struct {
	GoalWeight   float64   `json:"goalWeight"`
	GoalDeadline string    `json:"goalDeadline"`
	Unit         string    `json:"unit"`
	Height       float64   `json:"height"`
	Entries      []Entry   `json:"entries"`
}

// ─── Colors ──────────────────────────────────────────────────────────────────

const (
	reset  = "\x1b[0m"
	bright = "\x1b[1m"
	dim    = "\x1b[2m"
	red    = "\x1b[31m"
	green  = "\x1b[32m"
	yellow = "\x1b[33m"
	blue   = "\x1b[34m"
	magenta = "\x1b[35m"
	cyan   = "\x1b[36m"
)

func c(str, color string) string {
	return color + str + reset
}

// ─── Config ──────────────────────────────────────────────────────────────────

const (
	defaultGoal    = 75.0
	defaultUnit    = "kg"
	defaultHeight  = 175.0
)

// ─── Data Manager ──────────────────────────────────────────────────────────

type WeightTracker struct {
	goalWeight   float64
	goalDeadline string
	unit         string
	height       float64
	entries      []Entry
	file         string
	reader       *bufio.Reader
}

func NewWeightTracker() *WeightTracker {
	home, _ := os.UserHomeDir()
	dir := filepath.Join(home, ".weight_tracker")
	os.MkdirAll(dir, 0755)
	file := filepath.Join(dir, "data.json")
	w := &WeightTracker{file: file, reader: bufio.NewReader(os.Stdin)}
	w.load()
	return w
}

func (w *WeightTracker) load() {
	if _, err := os.Stat(w.file); os.IsNotExist(err) {
		w.goalWeight = defaultGoal
		w.goalDeadline = defaultDeadline()
		w.unit = defaultUnit
		w.height = defaultHeight
		w.entries = []Entry{}
		return
	}
	raw, err := os.ReadFile(w.file)
	if err != nil {
		w.goalWeight = defaultGoal
		w.goalDeadline = defaultDeadline()
		w.unit = defaultUnit
		w.height = defaultHeight
		w.entries = []Entry{}
		return
	}
	var data Data
	if err := json.Unmarshal(raw, &data); err != nil {
		w.goalWeight = defaultGoal
		w.goalDeadline = defaultDeadline()
		w.unit = defaultUnit
		w.height = defaultHeight
		w.entries = []Entry{}
		return
	}
	w.goalWeight = data.GoalWeight
	if w.goalWeight <= 0 {
		w.goalWeight = defaultGoal
	}
	w.goalDeadline = data.GoalDeadline
	if w.goalDeadline == "" {
		w.goalDeadline = defaultDeadline()
	}
	w.unit = data.Unit
	if w.unit == "" {
		w.unit = defaultUnit
	}
	w.height = data.Height
	if w.height <= 0 {
		w.height = defaultHeight
	}
	w.entries = data.Entries
	if w.entries == nil {
		w.entries = []Entry{}
	}
}

func defaultDeadline() string {
	return time.Now().AddDate(0, 0, 90).Format("2006-01-02")
}

func (w *WeightTracker) save() {
	data := Data{
		GoalWeight:   w.goalWeight,
		GoalDeadline: w.goalDeadline,
		Unit:         w.unit,
		Height:       w.height,
		Entries:      w.entries,
	}
	raw, _ := json.MarshalIndent(data, "", "  ")
	os.WriteFile(w.file, raw, 0644)
}

func (w *WeightTracker) today() string {
	return time.Now().Format("2006-01-02")
}

func (w *WeightTracker) getTodayEntry() *Entry {
	today := w.today()
	for i := range w.entries {
		if w.entries[i].Date == today {
			return &w.entries[i]
		}
	}
	return nil
}

func (w *WeightTracker) getTodayWeight() *float64 {
	e := w.getTodayEntry()
	if e == nil {
		return nil
	}
	return &e.Weight
}

func (w *WeightTracker) getAllEntriesSorted() []Entry {
	sorted := make([]Entry, len(w.entries))
	copy(sorted, w.entries)
	sort.Slice(sorted, func(i, j int) bool {
		return sorted[i].Date < sorted[j].Date
	})
	return sorted
}

func (w *WeightTracker) getWeights() []float64 {
	sorted := w.getAllEntriesSorted()
	weights := []float64{}
	for _, e := range sorted {
		weights = append(weights, e.Weight)
	}
	return weights
}

func (w *WeightTracker) progressBar(current, goal float64, width int) string {
	if goal <= 0 {
		return "⚠️  Goal not set"
	}
	weights := w.getWeights()
	if len(weights) == 0 {
		return "No data"
	}
	start := weights[0]
	var ratio float64
	if current >= start {
		ratio = math.Max(0, math.Min(1, (current-goal)/(start-goal)))
	} else {
		ratio = math.Max(0, math.Min(1, (start-current)/(start-goal)))
	}
	filled := int(ratio * float64(width))
	bar := strings.Repeat("█", filled) + strings.Repeat("░", width-filled)
	return fmt.Sprintf("[%s] %.1f%%", bar, ratio*100)
}

func (w *WeightTracker) ask(prompt string) string {
	fmt.Print(prompt)
	line, _ := w.reader.ReadString('\n')
	return strings.TrimSpace(line)
}

func (w *WeightTracker) askFloat(prompt string) float64 {
	for {
		ans := w.ask(prompt)
		if val, err := strconv.ParseFloat(ans, 64); err == nil {
			return val
		}
		fmt.Println(c("❌ Please enter a number.", red))
	}
}

func (w *WeightTracker) askString(prompt string) string {
	return w.ask(prompt)
}

func (w *WeightTracker) askConfirm(prompt string) bool {
	ans := w.ask(prompt + " (yes/no): ")
	return strings.ToLower(ans) == "yes"
}

// ─── Core Actions ──────────────────────────────────────────────────────────

func (w *WeightTracker) addEntry(weight float64, notes string) error {
	if weight <= 0 {
		return fmt.Errorf("weight must be positive")
	}
	today := w.today()
	e := w.getTodayEntry()
	if e != nil {
		e.Weight = weight
		e.Notes = notes
	} else {
		w.entries = append(w.entries, Entry{Date: today, Weight: weight, Notes: notes})
	}
	w.save()
	fmt.Printf(c("✅ Weight logged: %.1f %s\n", green), weight, w.unit)
	return nil
}

func (w *WeightTracker) showToday() {
	weight := w.getTodayWeight()
	if weight == nil {
		fmt.Println(c("No weight logged today.", yellow))
		return
	}
	diff := *weight - w.goalWeight
	status := "above"
	color := red
	if diff < 0 {
		status = "below"
		color = green
	}
	fmt.Println("\n" + c(strings.Repeat("═", 50), dim))
	fmt.Println(c("⚖️ TODAY'S WEIGHT", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), dim))
	fmt.Printf("  Weight: %s\n", c(fmt.Sprintf("%.1f %s", *weight, w.unit), color))
	fmt.Printf("  Goal:   %.1f %s\n", w.goalWeight, w.unit)
	fmt.Printf("  You are %s your goal\n", c(fmt.Sprintf("%.1f %s %s", math.Abs(diff), w.unit, status), color))
	fmt.Printf("  Progress: %s\n", w.progressBar(*weight, w.goalWeight, 20))
	fmt.Println(c(strings.Repeat("═", 50), dim))
}

func (w *WeightTracker) showChart(days int) {
	entries := w.getAllEntriesSorted()
	if len(entries) == 0 {
		fmt.Println(c("No data to chart.", yellow))
		return
	}
	if len(entries) > days {
		entries = entries[len(entries)-days:]
	}
	weights := []float64{}
	dates := []string{}
	for _, e := range entries {
		weights = append(weights, e.Weight)
		dates = append(dates, e.Date[5:10])
	}
	minW, maxW := weights[0], weights[0]
	for _, w := range weights {
		if w < minW {
			minW = w
		}
		if w > maxW {
			maxW = w
		}
	}
	rangeW := maxW - minW
	if rangeW == 0 {
		fmt.Println(c("Weight is constant. No variation to chart.", yellow))
		return
	}
	height := 10
	chartWidth := 40
	norm := make([]int, len(weights))
	for i, w := range weights {
		norm[i] = int((w - minW) / rangeW * float64(height-1))
	}
	lines := []string{}
	for row := height - 1; row >= 0; row-- {
		line := ""
		for i, val := range norm {
			if val >= row {
				if i > 0 && norm[i-1] >= row {
					line += "─"
				} else {
					line += "┌"
				}
			} else {
				line += " "
			}
		}
		lines = append(lines, line)
	}
	step := len(dates) / 8
	if step < 1 {
		step = 1
	}
	xAxis := " "
	lastPos := 0
	for i := 0; i < len(dates); i += step {
		if i >= len(dates) {
			break
		}
		label := dates[i]
		pos := i
		if pos > lastPos {
			xAxis += strings.Repeat(" ", pos-lastPos)
		}
		xAxis += label
		lastPos = pos
	}
	if lastPos < len(dates)-1 {
		xAxis += strings.Repeat(" ", len(dates)-1-lastPos)
	}
	fmt.Printf("\n%s\n", c(fmt.Sprintf("📈 Weight Chart (last %d days)", len(entries)), bright+cyan))
	fmt.Println(strings.Join(lines, "\n"))
	fmt.Println(xAxis)
	fmt.Printf("Min: %.1f %s  Max: %.1f %s\n", minW, w.unit, maxW, w.unit)
}

func (w *WeightTracker) showStats() {
	if len(w.entries) == 0 {
		fmt.Println(c("📭 No data yet. Start tracking!", yellow))
		return
	}
	weights := w.getWeights()
	if len(weights) == 0 {
		fmt.Println(c("No weight data available.", yellow))
		return
	}
	total := len(weights)
	sum := 0.0
	for _, v := range weights {
		sum += v
	}
	avg := sum / float64(total)
	minW, maxW := weights[0], weights[0]
	for _, v := range weights {
		if v < minW {
			minW = v
		}
		if v > maxW {
			maxW = v
		}
	}
	last := weights[len(weights)-1]
	first := weights[0]
	change := last - first
	trend := "down"
	if change > 0 {
		trend = "up"
	}
	heightM := w.height / 100.0
	var bmi float64
	if heightM > 0 {
		bmi = last / (heightM * heightM)
	}
	bmiCat := bmiCategory(bmi)
	fmt.Println("\n📊 STATISTICS")
	fmt.Println(c(strings.Repeat("─", 30), dim))
	fmt.Printf("  Total Entries:  %d\n", total)
	fmt.Printf("  Current Weight: %s\n", c(fmt.Sprintf("%.1f %s", last, w.unit), green))
	fmt.Printf("  Goal Weight:    %.1f %s\n", w.goalWeight, w.unit)
	fmt.Printf("  Average:        %.1f %s\n", avg, w.unit)
	fmt.Printf("  Minimum:        %.1f %s\n", minW, w.unit)
	fmt.Printf("  Maximum:        %.1f %s\n", maxW, w.unit)
	sign := ""
	if change > 0 {
		sign = "+"
	}
	fmt.Printf("  Change:         %s%.1f %s (%s)\n", sign, change, w.unit, trend)
	fmt.Printf("  BMI:            %.1f (%s)\n", bmi, bmiCat)
}

func bmiCategory(bmi float64) string {
	if bmi < 18.5 {
		return "Underweight"
	}
	if bmi < 25 {
		return "Normal"
	}
	if bmi < 30 {
		return "Overweight"
	}
	return "Obese"
}

func (w *WeightTracker) setGoal(weight float64, deadline string) error {
	if weight <= 0 {
		return fmt.Errorf("goal weight must be positive")
	}
	w.goalWeight = weight
	if deadline != "" {
		if _, err := time.Parse("2006-01-02", deadline); err != nil {
			return fmt.Errorf("invalid date format, use YYYY-MM-DD")
		}
		w.goalDeadline = deadline
	} else {
		w.goalDeadline = defaultDeadline()
	}
	w.save()
	fmt.Printf(c("✅ Goal set to %.1f %s by %s\n", green), weight, w.unit, w.goalDeadline)
	return nil
}

func (w *WeightTracker) setHeight(height float64) error {
	if height <= 0 {
		return fmt.Errorf("height must be positive")
	}
	w.height = height
	w.save()
	fmt.Printf(c("✅ Height set to %.1f cm\n", green), height)
	return nil
}

func (w *WeightTracker) setUnit(unit string) error {
	if unit != "kg" && unit != "lbs" {
		return fmt.Errorf("unit must be 'kg' or 'lbs'")
	}
	w.unit = unit
	w.save()
	fmt.Printf(c("✅ Unit set to %s\n", green), unit)
	return nil
}

func (w *WeightTracker) clearData() {
	if !w.askConfirm("⚠️  Delete ALL data? This cannot be undone!") {
		return
	}
	w.entries = []Entry{}
	w.goalWeight = defaultGoal
	w.goalDeadline = defaultDeadline()
	w.save()
	fmt.Println(c("🗑️  All data cleared.", yellow))
}

// ─── Menu ──────────────────────────────────────────────────────────────────

func (w *WeightTracker) showMenu() {
	todayWeight := w.getTodayWeight()
	var weightStr string
	if todayWeight != nil {
		weightStr = fmt.Sprintf("%.1f", *todayWeight)
	} else {
		weightStr = "—"
	}
	progress := w.progressBar(func() float64 {
		if todayWeight != nil {
			return *todayWeight
		}
		return 0
	}(), w.goalWeight, 20)
	fmt.Println("\n" + c(strings.Repeat("═", 50), cyan))
	fmt.Println(c("⚖️ WEIGHT TRACKER", bright+cyan))
	fmt.Println(c(strings.Repeat("═", 50), cyan))
	fmt.Printf("  Today: %s %s / %.1f %s\n", weightStr, w.unit, w.goalWeight, w.unit)
	fmt.Printf("  Goal deadline: %s\n", w.goalDeadline)
	fmt.Printf("  Progress: %s\n", progress)
	fmt.Println(c(strings.Repeat("─", 50), dim))
	fmt.Println("  1. ⚖️ Log weight today")
	fmt.Println("  2. 📊 Today's progress")
	fmt.Println("  3. 📈 Show weight chart")
	fmt.Println("  4. 📊 Statistics")
	fmt.Println("  5. 🎯 Set goal weight & deadline")
	fmt.Println("  6. 📏 Set height")
	fmt.Println("  7. 🔄 Set unit (kg/lbs)")
	fmt.Println("  8. 🗑️  Clear all data")
	fmt.Println("  0. 🚪 Exit")
	fmt.Println(c(strings.Repeat("═", 50), cyan))
}

func (w *WeightTracker) run() {
	fmt.Print("\033[H\033[2J")
	fmt.Println(c("\n⚖️ Weight Tracker – Goal‑Based Fitness Manager", bright+cyan))
	fmt.Println(c("Track your weight, reach your goals!", dim))

	for {
		w.showMenu()
		choice := w.ask("Your choice: ")
		switch choice {
		case "1":
			weight := w.askFloat(fmt.Sprintf("Weight (%s): ", w.unit))
			notes := w.askString("Notes (optional): ")
			w.addEntry(weight, notes)
		case "2":
			w.showToday()
		case "3":
			w.showChart(30)
		case "4":
			w.showStats()
		case "5":
			weight := w.askFloat(fmt.Sprintf("Goal weight (%s): ", w.unit))
			deadline := w.askString("Deadline (YYYY-MM-DD) [leave empty for default]: ")
			if err := w.setGoal(weight, deadline); err != nil {
				fmt.Printf("%s\n", c("❌ "+err.Error(), red))
			}
		case "6":
			height := w.askFloat("Height (cm): ")
			if err := w.setHeight(height); err != nil {
				fmt.Printf("%s\n", c("❌ "+err.Error(), red))
			}
		case "7":
			unit := w.askString("Unit (kg/lbs): ")
			if err := w.setUnit(strings.ToLower(unit)); err != nil {
				fmt.Printf("%s\n", c("❌ "+err.Error(), red))
			}
		case "8":
			w.clearData()
		case "0":
			fmt.Println(c("👋 Stay fit! Goodbye!", cyan))
			return
		default:
			fmt.Println(c("❌ Invalid choice.", red))
		}
		if choice != "0" {
			fmt.Print("\nPress Enter to continue...")
			w.reader.ReadString('\n')
		}
	}
}

func main() {
	app := NewWeightTracker()
	app.run()
}
