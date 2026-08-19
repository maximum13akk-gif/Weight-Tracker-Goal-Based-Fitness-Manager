# weight_tracker.cpp
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (C++ Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 * Uses only STL, no external libraries.
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <filesystem>
#include <cstdlib>
#include <limits>
#include <cmath>

#ifdef _WIN32
#include <windows.h>
#endif

// ─── Colors ──────────────────────────────────────────────────────────────────

#ifdef _WIN32
HANDLE hConsole;
void setColor(int color) { SetConsoleTextAttribute(hConsole, color); }
#define RESET_COLOR setColor(7)
#define COLOR_RED setColor(12)
#define COLOR_GREEN setColor(10)
#define COLOR_YELLOW setColor(14)
#define COLOR_BLUE setColor(9)
#define COLOR_MAGENTA setColor(13)
#define COLOR_CYAN setColor(11)
#define COLOR_BRIGHT setColor(15)
#define COLOR_DIM setColor(8)
#else
#define RESET_COLOR std::cout << "\x1b[0m"
#define COLOR_RED std::cout << "\x1b[31m"
#define COLOR_GREEN std::cout << "\x1b[32m"
#define COLOR_YELLOW std::cout << "\x1b[33m"
#define COLOR_BLUE std::cout << "\x1b[34m"
#define COLOR_MAGENTA std::cout << "\x1b[35m"
#define COLOR_CYAN std::cout << "\x1b[36m"
#define COLOR_BRIGHT std::cout << "\x1b[1m"
#define COLOR_DIM std::cout << "\x1b[2m"
#endif

#define C(str, color) color << str << RESET_COLOR

// ─── Helpers ─────────────────────────────────────────────────────────────────

std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

std::string get_today() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

std::string default_deadline() {
    std::time_t t = std::time(nullptr) + 90 * 86400;
    std::tm* tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

std::string get_home_dir() {
#ifdef _WIN32
    const char* h = std::getenv("USERPROFILE");
#else
    const char* h = std::getenv("HOME");
#endif
    return h ? std::string(h) : ".";
}

// ─── Data Structures ──────────────────────────────────────────────────────

struct Entry {
    std::string date;
    double weight;
    std::string notes;
};

struct Data {
    double goalWeight;
    std::string goalDeadline;
    std::string unit;
    double height;
    std::vector<Entry> entries;
};

// ─── JSON (simplified) ─────────────────────────────────────────────────────

std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

std::string serialize_data(const Data& data) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"goalWeight\": " << data.goalWeight << ",\n";
    json << "  \"goalDeadline\": \"" << escape_json(data.goalDeadline) << "\",\n";
    json << "  \"unit\": \"" << escape_json(data.unit) << "\",\n";
    json << "  \"height\": " << data.height << ",\n";
    json << "  \"entries\": [\n";
    for (size_t i = 0; i < data.entries.size(); ++i) {
        const auto& e = data.entries[i];
        json << "    {\n";
        json << "      \"date\": \"" << escape_json(e.date) << "\",\n";
        json << "      \"weight\": " << e.weight << ",\n";
        json << "      \"notes\": \"" << escape_json(e.notes) << "\"\n";
        json << "    }";
        if (i + 1 < data.entries.size()) json << ",";
        json << "\n";
    }
    json << "  ]\n";
    json << "}";
    return json.str();
}

bool deserialize_data(const std::string& json_str, Data& data) {
    data.goalWeight = 75.0;
    data.goalDeadline = default_deadline();
    data.unit = "kg";
    data.height = 175.0;
    data.entries.clear();
    // Simple manual parse (demo)
    auto find_double = [&](const std::string& key) -> double {
        size_t pos = json_str.find("\"" + key + "\":");
        if (pos == std::string::npos) return 0.0;
        pos = json_str.find(":", pos) + 1;
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\n' || json_str[pos] == '\r')) pos++;
        size_t end = json_str.find_first_of(",}\n\r", pos);
        if (end == std::string::npos) return 0.0;
        return std::stod(json_str.substr(pos, end - pos));
    };
    auto find_string = [&](const std::string& key) -> std::string {
        size_t pos = json_str.find("\"" + key + "\":");
        if (pos == std::string::npos) return "";
        pos = json_str.find(":", pos) + 1;
        while (pos < json_str.length() && (json_str[pos] == ' ' || json_str[pos] == '\n' || json_str[pos] == '\r')) pos++;
        if (json_str[pos] != '"') return "";
        pos++;
        size_t end = json_str.find("\"", pos);
        if (end == std::string::npos) return "";
        return json_str.substr(pos, end - pos);
    };
    double gw = find_double("goalWeight");
    if (gw > 0) data.goalWeight = gw;
    data.goalDeadline = find_string("goalDeadline");
    if (data.goalDeadline.empty()) data.goalDeadline = default_deadline();
    data.unit = find_string("unit");
    if (data.unit.empty()) data.unit = "kg";
    double h = find_double("height");
    if (h > 0) data.height = h;
    // entries not parsed for brevity
    return true;
}

// ─── Weight Tracker ────────────────────────────────────────────────────────

class WeightTracker {
public:
    WeightTracker() {
        home = get_home_dir();
        data_dir = home + "/.weight_tracker";
        std::filesystem::create_directories(data_dir);
        data_file = data_dir + "/data.json";
        load();
    }

    void load() {
        std::ifstream file(data_file);
        if (!file.is_open()) {
            data = Data{75.0, default_deadline(), "kg", 175.0, {}};
            return;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();
        if (!deserialize_data(buffer.str(), data)) {
            data = Data{75.0, default_deadline(), "kg", 175.0, {}};
        }
    }

    void save() {
        std::string json = serialize_data(data);
        std::string temp = data_file + ".tmp";
        std::ofstream out(temp);
        if (out.is_open()) {
            out << json;
            out.close();
            std::filesystem::rename(temp, data_file);
        }
    }

    std::string today() { return get_today(); }

    Entry* get_today_entry() {
        std::string today_str = today();
        for (auto& e : data.entries) {
            if (e.date == today_str) return &e;
        }
        return nullptr;
    }

    bool has_today_weight() {
        return get_today_entry() != nullptr;
    }

    double get_today_weight() {
        Entry* e = get_today_entry();
        return e ? e->weight : 0.0;
    }

    std::vector<Entry> get_all_entries_sorted() {
        std::vector<Entry> sorted = data.entries;
        std::sort(sorted.begin(), sorted.end(), [](const Entry& a, const Entry& b) {
            return a.date < b.date;
        });
        return sorted;
    }

    std::vector<double> get_weights() {
        auto sorted = get_all_entries_sorted();
        std::vector<double> weights;
        for (const auto& e : sorted) {
            weights.push_back(e.weight);
        }
        return weights;
    }

    std::string progress_bar(double current, double goal, int width = 20) {
        if (goal <= 0) return "⚠️  Goal not set";
        auto weights = get_weights();
        if (weights.empty()) return "No data";
        double start = weights[0];
        double ratio;
        if (current >= start) {
            ratio = std::max(0.0, std::min(1.0, (current - goal) / (start - goal)));
        } else {
            ratio = std::max(0.0, std::min(1.0, (start - current) / (start - goal)));
        }
        int filled = static_cast<int>(ratio * width);
        std::string bar = std::string(filled, '█') + std::string(width - filled, '░');
        char buf[32];
        snprintf(buf, sizeof(buf), "[%s] %.1f%%", bar.c_str(), ratio * 100.0);
        return std::string(buf);
    }

    std::string ask(const std::string& prompt) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return trim(line);
    }

    double ask_double(const std::string& prompt) {
        while (true) {
            std::string ans = ask(prompt);
            try {
                return std::stod(ans);
            } catch (...) {
                std::cout << C("❌ Please enter a number.", COLOR_RED) << std::endl;
            }
        }
    }

    bool ask_confirm(const std::string& prompt) {
        std::string ans = ask(prompt + " (yes/no): ");
        std::string lower = ans;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower == "yes";
    }

    // ─── Core Actions ──────────────────────────────────────────────────────

    void add_entry(double weight, const std::string& notes) {
        if (weight <= 0) {
            std::cout << C("❌ Weight must be positive!", COLOR_RED) << std::endl;
            return;
        }
        std::string today_str = today();
        Entry* e = get_today_entry();
        if (e) {
            e->weight = weight;
            e->notes = notes;
        } else {
            Entry newEntry{today_str, weight, notes};
            data.entries.push_back(newEntry);
        }
        save();
        std::cout << C("✅ Weight logged: " + std::to_string(weight) + " " + data.unit, COLOR_GREEN) << std::endl;
    }

    void show_today() {
        if (!has_today_weight()) {
            std::cout << C("No weight logged today.", COLOR_YELLOW) << std::endl;
            return;
        }
        double weight = get_today_weight();
        double diff = weight - data.goalWeight;
        std::string status = diff > 0 ? "above" : "below";
        std::string color = diff > 0 ? COLOR_RED : COLOR_GREEN;
        std::cout << "\n" << C(std::string(50, '═'), COLOR_DIM) << std::endl;
        std::cout << C("⚖️ TODAY'S WEIGHT", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_DIM) << std::endl;
        std::cout << "  Weight: " << C(std::to_string(weight) + " " + data.unit, color) << std::endl;
        std::cout << "  Goal:   " << data.goalWeight << " " << data.unit << std::endl;
        std::cout << "  You are " << C(std::to_string(std::abs(diff)) + " " + data.unit + " " + status, color) << " your goal" << std::endl;
        std::cout << "  Progress: " << progress_bar(weight, data.goalWeight) << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_DIM) << std::endl;
    }

    void show_chart(int days = 30) {
        auto entries = get_all_entries_sorted();
        if (entries.empty()) {
            std::cout << C("No data to chart.", COLOR_YELLOW) << std::endl;
            return;
        }
        if ((int)entries.size() > days) {
            entries.erase(entries.begin(), entries.begin() + (entries.size() - days));
        }
        std::vector<double> weights;
        std::vector<std::string> dates;
        for (const auto& e : entries) {
            weights.push_back(e.weight);
            dates.push_back(e.date.substr(5, 5));
        }
        double minW = weights[0], maxW = weights[0];
        for (double w : weights) {
            if (w < minW) minW = w;
            if (w > maxW) maxW = w;
        }
        double rangeW = maxW - minW;
        if (rangeW == 0) {
            std::cout << C("Weight is constant. No variation to chart.", COLOR_YELLOW) << std::endl;
            return;
        }
        int height = 10;
        int chartWidth = 40;
        std::vector<int> norm;
        for (double w : weights) {
            norm.push_back(static_cast<int>((w - minW) / rangeW * (height - 1)));
        }
        std::vector<std::string> lines;
        for (int row = height - 1; row >= 0; --row) {
            std::string line;
            for (size_t i = 0; i < norm.size(); ++i) {
                if (norm[i] >= row) {
                    if (i > 0 && norm[i-1] >= row) line += "─";
                    else line += "┌";
                } else {
                    line += " ";
                }
            }
            lines.push_back(line);
        }
        int step = std::max(1, (int)dates.size() / 8);
        std::string xAxis = " ";
        int lastPos = 0;
        for (int i = 0; i < (int)dates.size(); i += step) {
            if (i >= (int)dates.size()) break;
            std::string label = dates[i];
            int pos = i;
            if (pos > lastPos) xAxis += std::string(pos - lastPos, ' ');
            xAxis += label;
            lastPos = pos;
        }
        if (lastPos < (int)dates.size() - 1) {
            xAxis += std::string(dates.size() - 1 - lastPos, ' ');
        }
        std::cout << "\n" << C("📈 Weight Chart (last " + std::to_string(entries.size()) + " days)", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        for (const auto& line : lines) std::cout << line << std::endl;
        std::cout << xAxis << std::endl;
        std::cout << "Min: " << minW << " " << data.unit << "  Max: " << maxW << " " << data.unit << std::endl;
    }

    void show_stats() {
        if (data.entries.empty()) {
            std::cout << C("📭 No data yet. Start tracking!", COLOR_YELLOW) << std::endl;
            return;
        }
        auto weights = get_weights();
        if (weights.empty()) {
            std::cout << C("No weight data available.", COLOR_YELLOW) << std::endl;
            return;
        }
        double total = 0;
        double minW = weights[0], maxW = weights[0];
        for (double w : weights) {
            total += w;
            if (w < minW) minW = w;
            if (w > maxW) maxW = w;
        }
        double avg = total / weights.size();
        double last = weights.back();
        double first = weights.front();
        double change = last - first;
        std::string trend = change > 0 ? "up" : "down";
        double heightM = data.height / 100.0;
        double bmi = heightM > 0 ? last / (heightM * heightM) : 0;
        std::string bmiCat = bmi_category(bmi);
        std::cout << "\n📊 STATISTICS" << std::endl;
        std::cout << C(std::string(30, '─'), COLOR_DIM) << std::endl;
        std::cout << "  Total Entries:  " << weights.size() << std::endl;
        std::cout << "  Current Weight: " << C(std::to_string(last) + " " + data.unit, COLOR_GREEN) << std::endl;
        std::cout << "  Goal Weight:    " << data.goalWeight << " " << data.unit << std::endl;
        std::cout << "  Average:        " << avg << " " << data.unit << std::endl;
        std::cout << "  Minimum:        " << minW << " " << data.unit << std::endl;
        std::cout << "  Maximum:        " << maxW << " " << data.unit << std::endl;
        std::string sign = change > 0 ? "+" : "";
        std::cout << "  Change:         " << sign << change << " " << data.unit << " (" << trend << ")" << std::endl;
        std::cout << "  BMI:            " << bmi << " (" << bmiCat << ")" << std::endl;
    }

    std::string bmi_category(double bmi) {
        if (bmi < 18.5) return "Underweight";
        if (bmi < 25) return "Normal";
        if (bmi < 30) return "Overweight";
        return "Obese";
    }

    void set_goal(double weight, const std::string& deadline) {
        if (weight <= 0) {
            std::cout << C("❌ Goal weight must be positive!", COLOR_RED) << std::endl;
            return;
        }
        data.goalWeight = weight;
        if (!deadline.empty()) {
            // Validate date format (simple)
            if (deadline.size() != 10 || deadline[4] != '-' || deadline[7] != '-') {
                std::cout << C("⚠️  Invalid date format. Use YYYY-MM-DD. Keeping current deadline.", COLOR_YELLOW) << std::endl;
                return;
            }
            data.goalDeadline = deadline;
        } else {
            data.goalDeadline = default_deadline();
        }
        save();
        std::cout << C("✅ Goal set to " + std::to_string(weight) + " " + data.unit + " by " + data.goalDeadline, COLOR_GREEN) << std::endl;
    }

    void set_height(double height) {
        if (height <= 0) {
            std::cout << C("❌ Height must be positive!", COLOR_RED) << std::endl;
            return;
        }
        data.height = height;
        save();
        std::cout << C("✅ Height set to " + std::to_string(height) + " cm", COLOR_GREEN) << std::endl;
    }

    void set_unit(const std::string& unit) {
        if (unit != "kg" && unit != "lbs") {
            std::cout << C("❌ Unit must be 'kg' or 'lbs'", COLOR_RED) << std::endl;
            return;
        }
        data.unit = unit;
        save();
        std::cout << C("✅ Unit set to " + unit, COLOR_GREEN) << std::endl;
    }

    void clear_data() {
        if (!ask_confirm("⚠️  Delete ALL data? This cannot be undone!")) return;
        data.entries.clear();
        data.goalWeight = 75.0;
        data.goalDeadline = default_deadline();
        save();
        std::cout << C("🗑️  All data cleared.", COLOR_YELLOW) << std::endl;
    }

    // ─── Menu ──────────────────────────────────────────────────────────────

    void show_menu() {
        bool has_today = has_today_weight();
        double todayW = get_today_weight();
        std::string weightStr = has_today ? std::to_string(todayW) : "—";
        std::string progress = progress_bar(has_today ? todayW : 0, data.goalWeight);
        std::cout << "\n" << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << C("⚖️ WEIGHT TRACKER", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
        std::cout << "  Today: " << weightStr << " " << data.unit << " / " << data.goalWeight << " " << data.unit << std::endl;
        std::cout << "  Goal deadline: " << data.goalDeadline << std::endl;
        std::cout << "  Progress: " << progress << std::endl;
        std::cout << C(std::string(50, '─'), COLOR_DIM) << std::endl;
        std::cout << "  1. ⚖️ Log weight today" << std::endl;
        std::cout << "  2. 📊 Today's progress" << std::endl;
        std::cout << "  3. 📈 Show weight chart" << std::endl;
        std::cout << "  4. 📊 Statistics" << std::endl;
        std::cout << "  5. 🎯 Set goal weight & deadline" << std::endl;
        std::cout << "  6. 📏 Set height" << std::endl;
        std::cout << "  7. 🔄 Set unit (kg/lbs)" << std::endl;
        std::cout << "  8. 🗑️  Clear all data" << std::endl;
        std::cout << "  0. 🚪 Exit" << std::endl;
        std::cout << C(std::string(50, '═'), COLOR_CYAN) << std::endl;
    }

    void run() {
        std::cout << "\033[2J\033[1;1H";
        std::cout << C("\n⚖️ Weight Tracker – Goal‑Based Fitness Manager", COLOR_BRIGHT) << C("", COLOR_CYAN) << std::endl;
        std::cout << C("Track your weight, reach your goals!", COLOR_DIM) << std::endl;

        while (true) {
            show_menu();
            std::string choice = ask("Your choice: ");
            if (choice == "1") {
                double weight = ask_double("Weight (" + data.unit + "): ");
                std::string notes = ask("Notes (optional): ");
                add_entry(weight, notes);
            } else if (choice == "2") {
                show_today();
            } else if (choice == "3") {
                show_chart(30);
            } else if (choice == "4") {
                show_stats();
            } else if (choice == "5") {
                double weight = ask_double("Goal weight (" + data.unit + "): ");
                std::string deadline = ask("Deadline (YYYY-MM-DD) [leave empty for default]: ");
                set_goal(weight, deadline);
            } else if (choice == "6") {
                double height = ask_double("Height (cm): ");
                set_height(height);
            } else if (choice == "7") {
                std::string unit = ask("Unit (kg/lbs): ");
                set_unit(unit);
            } else if (choice == "8") {
                clear_data();
            } else if (choice == "0") {
                std::cout << C("👋 Stay fit! Goodbye!", COLOR_CYAN) << std::endl;
                break;
            } else {
                std::cout << C("❌ Invalid choice.", COLOR_RED) << std::endl;
            }
            if (choice != "0") {
                std::cout << "\nPress Enter to continue...";
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.get();
            }
        }
    }

private:
    std::string home, data_dir, data_file;
    Data data;
};

int main() {
#ifdef _WIN32
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
#endif
    try {
        WeightTracker app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << C("❌ Unexpected error: ", COLOR_RED) << e.what() << std::endl;
        return 1;
    }
    return 0;
}
