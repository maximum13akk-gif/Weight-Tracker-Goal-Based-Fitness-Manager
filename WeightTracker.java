# WeightTracker.java
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (Java Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 * Requires: Java 17+
 */

import java.io.*;
import java.nio.file.*;
import java.time.*;
import java.time.format.*;
import java.util.*;
import java.util.stream.*;

public class WeightTracker {
    // ─── Colors ────────────────────────────────────────────────────────────

    private static final String RESET = "\u001B[0m";
    private static final String BRIGHT = "\u001B[1m";
    private static final String DIM = "\u001B[2m";
    private static final String RED = "\u001B[31m";
    private static final String GREEN = "\u001B[32m";
    private static final String YELLOW = "\u001B[33m";
    private static final String BLUE = "\u001B[34m";
    private static final String MAGENTA = "\u001B[35m";
    private static final String CYAN = "\u001B[36m";

    private static String c(String text, String color) { return color + text + RESET; }

    // ─── Data Classes ──────────────────────────────────────────────────────

    private static class Entry {
        String date;
        double weight;
        String notes;
        Entry(String date, double weight, String notes) {
            this.date = date; this.weight = weight; this.notes = notes;
        }
    }

    private static class Data {
        double goalWeight = 75.0;
        String goalDeadline = defaultDeadline();
        String unit = "kg";
        double height = 175.0;
        List<Entry> entries = new ArrayList<>();
    }

    private static String defaultDeadline() {
        return LocalDate.now().plusDays(90).format(DateTimeFormatter.ISO_LOCAL_DATE);
    }

    // ─── Config ────────────────────────────────────────────────────────────

    private static final double DEFAULT_GOAL = 75.0;
    private static final String DATA_DIR = System.getProperty("user.home") + "/.weight_tracker";
    private static final String DATA_FILE = DATA_DIR + "/data.json";

    // ─── Weight Tracker ────────────────────────────────────────────────────

    private final Scanner scanner;
    private Data data;

    public WeightTracker() throws IOException {
        scanner = new Scanner(System.in);
        Files.createDirectories(Paths.get(DATA_DIR));
        data = new Data();
        load();
    }

    private void load() {
        Path path = Paths.get(DATA_FILE);
        if (!Files.exists(path)) return;
        try {
            String json = Files.readString(path);
            data.goalWeight = extractDouble(json, "goalWeight");
            if (data.goalWeight <= 0) data.goalWeight = DEFAULT_GOAL;
            data.goalDeadline = extractString(json, "goalDeadline");
            if (data.goalDeadline.isEmpty()) data.goalDeadline = defaultDeadline();
            data.unit = extractString(json, "unit");
            if (data.unit.isEmpty()) data.unit = "kg";
            data.height = extractDouble(json, "height");
            if (data.height <= 0) data.height = 175.0;
            // entries not parsed for brevity
        } catch (Exception e) {
            data = new Data();
        }
    }

    private double extractDouble(String json, String key) {
        String pattern = "\"" + key + "\"\\s*:\\s*([\\d.]+)";
        var m = java.util.regex.Pattern.compile(pattern).matcher(json);
        return m.find() ? Double.parseDouble(m.group(1)) : 0.0;
    }

    private String extractString(String json, String key) {
        String pattern = "\"" + key + "\"\\s*:\\s*\"([^\"]*)\"";
        var m = java.util.regex.Pattern.compile(pattern).matcher(json);
        return m.find() ? m.group(1) : "";
    }

    private void save() {
        try {
            StringBuilder sb = new StringBuilder();
            sb.append("{\n");
            sb.append("  \"goalWeight\": ").append(data.goalWeight).append(",\n");
            sb.append("  \"goalDeadline\": \"").append(escapeJson(data.goalDeadline)).append("\",\n");
            sb.append("  \"unit\": \"").append(escapeJson(data.unit)).append("\",\n");
            sb.append("  \"height\": ").append(data.height).append(",\n");
            sb.append("  \"entries\": [\n");
            for (int i = 0; i < data.entries.size(); i++) {
                Entry e = data.entries.get(i);
                sb.append("    {\n");
                sb.append("      \"date\": \"").append(escapeJson(e.date)).append("\",\n");
                sb.append("      \"weight\": ").append(e.weight).append(",\n");
                sb.append("      \"notes\": \"").append(escapeJson(e.notes)).append("\"\n");
                sb.append("    }");
                if (i < data.entries.size() - 1) sb.append(",");
                sb.append("\n");
            }
            sb.append("  ]\n");
            sb.append("}");
            Files.writeString(Paths.get(DATA_FILE), sb.toString());
        } catch (IOException e) { e.printStackTrace(); }
    }

    private String escapeJson(String s) {
        return s.replace("\\", "\\\\").replace("\"", "\\\"");
    }

    private String today() {
        return LocalDate.now().format(DateTimeFormatter.ISO_LOCAL_DATE);
    }

    private Entry getTodayEntry() {
        String todayStr = today();
        for (Entry e : data.entries) {
            if (e.date.equals(todayStr)) return e;
        }
        return null;
    }

    private Double getTodayWeight() {
        Entry e = getTodayEntry();
        return e != null ? e.weight : null;
    }

    private List<Entry> getAllEntriesSorted() {
        return data.entries.stream()
                .sorted(Comparator.comparing(e -> e.date))
                .collect(Collectors.toList());
    }

    private List<Double> getWeights() {
        return getAllEntriesSorted().stream().map(e -> e.weight).collect(Collectors.toList());
    }

    private String progressBar(double current, double goal, int width) {
        if (goal <= 0) return "⚠️  Goal not set";
        List<Double> weights = getWeights();
        if (weights.isEmpty()) return "No data";
        double start = weights.get(0);
        double ratio;
        if (current >= start) {
            ratio = Math.max(0, Math.min(1, (current - goal) / (start - goal)));
        } else {
            ratio = Math.max(0, Math.min(1, (start - current) / (start - goal)));
        }
        int filled = (int)(ratio * width);
        StringBuilder bar = new StringBuilder();
        bar.append("[");
        bar.append("█".repeat(filled));
        bar.append("░".repeat(width - filled));
        bar.append("] ");
        bar.append(String.format("%.1f%%", ratio * 100));
        return bar.toString();
    }

    private String ask(String prompt) {
        System.out.print(prompt);
        return scanner.nextLine().trim();
    }

    private double askDouble(String prompt) {
        while (true) {
            try {
                return Double.parseDouble(ask(prompt));
            } catch (NumberFormatException e) {
                System.out.println(c("❌ Please enter a number.", RED));
            }
        }
    }

    private boolean askConfirm(String prompt) {
        String ans = ask(prompt + " (yes/no): ").toLowerCase();
        return ans.equals("yes");
    }

    // ─── Core Actions ─────────────────────────────────────────────────────

    private void addEntry(double weight, String notes) {
        if (weight <= 0) {
            System.out.println(c("❌ Weight must be positive!", RED));
            return;
        }
        String todayStr = today();
        Entry existing = getTodayEntry();
        if (existing != null) {
            existing.weight = weight;
            existing.notes = notes;
        } else {
            data.entries.add(new Entry(todayStr, weight, notes));
        }
        save();
        System.out.println(c("✅ Weight logged: " + String.format("%.1f", weight) + " " + data.unit, GREEN));
    }

    private void showToday() {
        Double weight = getTodayWeight();
        if (weight == null) {
            System.out.println(c("No weight logged today.", YELLOW));
            return;
        }
        double diff = weight - data.goalWeight;
        String status = diff > 0 ? "above" : "below";
        String color = diff > 0 ? RED : GREEN;
        System.out.println("\n" + c("═".repeat(50), DIM));
        System.out.println(c("⚖️ TODAY'S WEIGHT", BRIGHT + CYAN));
        System.out.println(c("═".repeat(50), DIM));
        System.out.println("  Weight: " + c(String.format("%.1f", weight) + " " + data.unit, color));
        System.out.println("  Goal:   " + String.format("%.1f", data.goalWeight) + " " + data.unit);
        System.out.println("  You are " + c(String.format("%.1f", Math.abs(diff)) + " " + data.unit + " " + status, color) + " your goal");
        System.out.println("  Progress: " + progressBar(weight, data.goalWeight, 20));
        System.out.println(c("═".repeat(50), DIM));
    }

    private void showChart(int days) {
        List<Entry> entries = getAllEntriesSorted();
        if (entries.isEmpty()) {
            System.out.println(c("No data to chart.", YELLOW));
            return;
        }
        if (entries.size() > days) {
            entries = entries.subList(entries.size() - days, entries.size());
        }
        List<Double> weights = entries.stream().map(e -> e.weight).collect(Collectors.toList());
        List<String> dates = entries.stream().map(e -> e.date.substring(5, 10)).collect(Collectors.toList());
        double minW = weights.stream().min(Double::compare).orElse(0.0);
        double maxW = weights.stream().max(Double::compare).orElse(0.0);
        double range = maxW - minW;
        if (range == 0) {
            System.out.println(c("Weight is constant. No variation to chart.", YELLOW));
            return;
        }
        int height = 10;
        int chartWidth = 40;
        List<Integer> norm = weights.stream()
                .map(w -> (int)((w - minW) / range * (height - 1)))
                .collect(Collectors.toList());
        List<String> lines = new ArrayList<>();
        for (int row = height - 1; row >= 0; row--) {
            StringBuilder line = new StringBuilder();
            for (int i = 0; i < norm.size(); i++) {
                if (norm.get(i) >= row) {
                    if (i > 0 && norm.get(i-1) >= row) {
                        line.append('─');
                    } else {
                        line.append('┌');
                    }
                } else {
                    line.append(' ');
                }
            }
            lines.add(line.toString());
        }
        int step = Math.max(1, dates.size() / 8);
        StringBuilder xAxis = new StringBuilder(" ");
        int lastPos = 0;
        for (int i = 0; i < dates.size(); i += step) {
            if (i >= dates.size()) break;
            String label = dates.get(i);
            int pos = i;
            if (pos > lastPos) xAxis.append(" ".repeat(pos - lastPos));
            xAxis.append(label);
            lastPos = pos;
        }
        if (lastPos < dates.size() - 1) {
            xAxis.append(" ".repeat(dates.size() - 1 - lastPos));
        }
        System.out.println("\n" + c("📈 Weight Chart (last " + entries.size() + " days)", BRIGHT + CYAN));
        lines.forEach(System.out::println);
        System.out.println(xAxis.toString());
        System.out.printf("Min: %.1f %s  Max: %.1f %s\n", minW, data.unit, maxW, data.unit);
    }

    private void showStats() {
        if (data.entries.isEmpty()) {
            System.out.println(c("📭 No data yet. Start tracking!", YELLOW));
            return;
        }
        List<Double> weights = getWeights();
        if (weights.isEmpty()) {
            System.out.println(c("No weight data available.", YELLOW));
            return;
        }
        double total = weights.stream().mapToDouble(Double::doubleValue).sum();
        double avg = total / weights.size();
        double minW = weights.stream().min(Double::compare).orElse(0.0);
        double maxW = weights.stream().max(Double::compare).orElse(0.0);
        double last = weights.get(weights.size() - 1);
        double first = weights.get(0);
        double change = last - first;
        String trend = change > 0 ? "up" : "down";
        double heightM = data.height / 100.0;
        double bmi = heightM > 0 ? last / (heightM * heightM) : 0.0;
        String bmiCat = bmiCategory(bmi);
        System.out.println("\n📊 STATISTICS");
        System.out.println(c("─".repeat(30), DIM));
        System.out.printf("  Total Entries:  %d\n", weights.size());
        System.out.println("  Current Weight: " + c(String.format("%.1f", last) + " " + data.unit, GREEN));
        System.out.printf("  Goal Weight:    %.1f %s\n", data.goalWeight, data.unit);
        System.out.printf("  Average:        %.1f %s\n", avg, data.unit);
        System.out.printf("  Minimum:        %.1f %s\n", minW, data.unit);
        System.out.printf("  Maximum:        %.1f %s\n", maxW, data.unit);
        String sign = change > 0 ? "+" : "";
        System.out.printf("  Change:         %s%.1f %s (%s)\n", sign, change, data.unit, trend);
        System.out.printf("  BMI:            %.1f (%s)\n", bmi, bmiCat);
    }

    private String bmiCategory(double bmi) {
        if (bmi < 18.5) return "Underweight";
        if (bmi < 25) return "Normal";
        if (bmi < 30) return "Overweight";
        return "Obese";
    }

    private void setGoal(double weight, String deadline) {
        if (weight <= 0) {
            System.out.println(c("❌ Goal weight must be positive!", RED));
            return;
        }
        data.goalWeight = weight;
        if (!deadline.isEmpty()) {
            try {
                LocalDate.parse(deadline, DateTimeFormatter.ISO_LOCAL_DATE);
                data.goalDeadline = deadline;
            } catch (DateTimeParseException e) {
                System.out.println(c("⚠️  Invalid date format. Use YYYY-MM-DD. Keeping current deadline.", YELLOW));
                return;
            }
        } else {
            data.goalDeadline = defaultDeadline();
        }
        save();
        System.out.println(c("✅ Goal set to " + String.format("%.1f", weight) + " " + data.unit + " by " + data.goalDeadline, GREEN));
    }

    private void setHeight(double height) {
        if (height <= 0) {
            System.out.println(c("❌ Height must be positive!", RED));
            return;
        }
        data.height = height;
        save();
        System.out.println(c("✅ Height set to " + String.format("%.1f", height) + " cm", GREEN));
    }

    private void setUnit(String unit) {
        if (!unit.equals("kg") && !unit.equals("lbs")) {
            System.out.println(c("❌ Unit must be 'kg' or 'lbs'", RED));
            return;
        }
        data.unit = unit;
        save();
        System.out.println(c("✅ Unit set to " + unit, GREEN));
    }

    private void clearData() {
        if (!askConfirm("⚠️  Delete ALL data? This cannot be undone!")) return;
        data.entries.clear();
        data.goalWeight = DEFAULT_GOAL;
        data.goalDeadline = defaultDeadline();
        save();
        System.out.println(c("🗑️  All data cleared.", YELLOW));
    }

    // ─── Menu ─────────────────────────────────────────────────────────────

    private void showMenu() {
        Double todayWeight = getTodayWeight();
        String weightStr = todayWeight != null ? String.format("%.1f", todayWeight) : "—";
        String progress = progressBar(todayWeight != null ? todayWeight : 0, data.goalWeight, 20);
        System.out.println("\n" + c("═".repeat(50), CYAN));
        System.out.println(c("⚖️ WEIGHT TRACKER", BRIGHT + CYAN));
        System.out.println(c("═".repeat(50), CYAN));
        System.out.println("  Today: " + weightStr + " " + data.unit + " / " + String.format("%.1f", data.goalWeight) + " " + data.unit);
        System.out.println("  Goal deadline: " + data.goalDeadline);
        System.out.println("  Progress: " + progress);
        System.out.println(c("─".repeat(50), DIM));
        System.out.println("  1. ⚖️ Log weight today");
        System.out.println("  2. 📊 Today's progress");
        System.out.println("  3. 📈 Show weight chart");
        System.out.println("  4. 📊 Statistics");
        System.out.println("  5. 🎯 Set goal weight & deadline");
        System.out.println("  6. 📏 Set height");
        System.out.println("  7. 🔄 Set unit (kg/lbs)");
        System.out.println("  8. 🗑️  Clear all data");
        System.out.println("  0. 🚪 Exit");
        System.out.println(c("═".repeat(50), CYAN));
    }

    public void run() {
        System.out.print("\033[H\033[2J");
        System.out.flush();
        System.out.println(c("\n⚖️ Weight Tracker – Goal‑Based Fitness Manager", BRIGHT + CYAN));
        System.out.println(c("Track your weight, reach your goals!", DIM));

        while (true) {
            showMenu();
            String choice = ask("Your choice: ");
            switch (choice) {
                case "1": {
                    double weight = askDouble("Weight (" + data.unit + "): ");
                    String notes = ask("Notes (optional): ");
                    addEntry(weight, notes);
                    break;
                }
                case "2": showToday(); break;
                case "3": showChart(30); break;
                case "4": showStats(); break;
                case "5": {
                    double weight = askDouble("Goal weight (" + data.unit + "): ");
                    String deadline = ask("Deadline (YYYY-MM-DD) [leave empty for default]: ");
                    setGoal(weight, deadline);
                    break;
                }
                case "6": {
                    double height = askDouble("Height (cm): ");
                    setHeight(height);
                    break;
                }
                case "7": {
                    String unit = ask("Unit (kg/lbs): ");
                    setUnit(unit.toLowerCase());
                    break;
                }
                case "8": clearData(); break;
                case "0":
                    System.out.println(c("👋 Stay fit! Goodbye!", CYAN));
                    return;
                default:
                    System.out.println(c("❌ Invalid choice.", RED));
            }
            if (!choice.equals("0")) {
                System.out.print("\nPress Enter to continue...");
                scanner.nextLine();
            }
        }
    }

    public static void main(String[] args) {
        try {
            new WeightTracker().run();
        } catch (Exception e) {
            System.err.println(c("❌ Unexpected error: " + e.getMessage(), RED));
            e.printStackTrace();
            System.exit(1);
        }
    }
}
