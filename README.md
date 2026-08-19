⚖️ Weight Tracker – Goal‑Based Fitness Manager
"Track your weight, set your goals, visualize your progress – and achieve the body you deserve!"

📋 Table of Contents
✨ Features

📁 Repository Structure

🚀 Quick Start

💻 Language Implementations

📊 Data Format

🤝 Contributing

📄 License

✨ Features
Feature	Description
⚖️ Log Weight	Record your weight (kg or lbs) with optional notes
🎯 Goal Setting	Set target weight and deadline date
📈 Progress Tracking	See how far you are from your goal with a visual bar
📊 ASCII Chart	Display a weight‑over‑time chart (last 30 days)
📉 Statistics	Show min, max, average, BMI (with height), and trend
🗓️ History	Review all weight entries sorted by date
📏 Unit Support	Switch between kg and lbs
💾 Persistence	All data stored in JSON format
🎨 Colorful CLI	Beautiful terminal output with emojis and progress indicators
⚡ Cross‑Platform	Works on Windows, macOS, and Linux
📁 Repository Structure
text
weight-tracker-goals/
├── README.md
├── python/
│   └── weight_tracker.py
├── javascript/
│   └── weight_tracker.js
├── typescript/
│   └── weight_tracker.ts
├── go/
│   └── weight_tracker.go
├── rust/
│   └── weight_tracker.rs
├── cpp/
│   └── weight_tracker.cpp
├── java/
│   └── WeightTracker.java
└── csharp/
    └── WeightTracker.cs
🚀 Quick Start
Prerequisites
Each language requires its respective runtime/compiler (see individual sections)

Clone & Run
bash
git clone https://github.com/yourusername/weight-tracker-goals.git
cd weight-tracker-goals
# Navigate to your language folder and run
💻 Language Implementations
1. 🐍 Python
bash
cd python
pip install rich
python weight_tracker.py
Requires: Python 3.8+

2. 🟨 JavaScript (Node.js)
bash
cd javascript
node weight_tracker.js
Requires: Node.js 16+

3. 🟦 TypeScript
bash
cd typescript
npm install -g ts-node
ts-node weight_tracker.ts
Requires: Node.js 16+, TypeScript

4. 🟩 Go
bash
cd go
go run weight_tracker.go
Requires: Go 1.18+

5. 🦀 Rust
bash
cd rust
cargo run
Requires: Rust 1.70+ (dependencies: serde, serde_json, chrono, colored)

6. ⚙️ C++
bash
cd cpp
g++ -std=c++17 weight_tracker.cpp -o weight_tracker
./weight_tracker
Requires: C++17 compiler

7. ☕ Java
bash
cd java
javac WeightTracker.java
java WeightTracker
Requires: JDK 17+

8. 🔷 C#
bash
cd csharp
dotnet run
Requires: .NET 6.0+

📊 Data Format
All implementations store data in ~/.weight_tracker/data.json:

json
{
  "goal_weight": 70.0,
  "goal_deadline": "2026-12-31",
  "unit": "kg",
  "height": 175.0,
  "entries": [
    {
      "date": "2026-08-19",
      "weight": 72.5,
      "notes": "feeling good"
    }
  ]
}
🤝 Contributing
Contributions are welcome! Please:

Fork the repository

Create a feature branch

Commit your changes

Open a Pull Request

📄 License
MIT © 2026 Weight Tracker Team
