# weight_tracker.js
/**
 * ⚖️ Weight Tracker Goals – Fitness Manager (Node.js Edition)
 * Features: log weight, set goal, progress, chart, stats, BMI, unit switch, JSON storage
 */

const fs = require('fs');
const path = require('path');
const os = require('os');
const readline = require('readline');

// ─── Colors ──────────────────────────────────────────────────────────────────

const colors = {
    reset: '\x1b[0m',
    bright: '\x1b[1m',
    dim: '\x1b[2m',
    red: '\x1b[31m',
    green: '\x1b[32m',
    yellow: '\x1b[33m',
    blue: '\x1b[34m',
    magenta: '\x1b[35m',
    cyan: '\x1b[36m',
};

const c = (str, color) => `${color}${str}${colors.reset}`;

// ─── Config ──────────────────────────────────────────────────────────────────

const CONFIG = {
    dataDir: path.join(os.homedir(), '.weight_tracker'),
    dataFile: 'data.json',
    defaultGoal: 75,
    defaultUnit: 'kg',
    defaultHeight: 175,
};

// ─── Data Manager ──────────────────────────────────────────────────────────

class WeightTracker {
    constructor() {
        this.rl = readline.createInterface({ input: process.stdin, output: process.stdout });
        this.data = this._load();
        this.goalWeight = this.data.goalWeight || CONFIG.defaultGoal;
        this.goalDeadline = this.data.goalDeadline || this._defaultDeadline();
        this.unit = this.data.unit || CONFIG.defaultUnit;
        this.height = this.data.height || CONFIG.defaultHeight;
        this.entries = this.data.entries || [];
    }

    _defaultDeadline() {
        const d = new Date();
        d.setDate(d.getDate() + 90);
        return d.toISOString().split('T')[0];
    }

    _getDataPath() {
        if (!fs.existsSync(CONFIG.dataDir)) fs.mkdirSync(CONFIG.dataDir, { recursive: true });
        return path.join(CONFIG.dataDir, CONFIG.dataFile);
    }

    _load() {
        const file = this._getDataPath();
        if (fs.existsSync(file)) {
            try {
                return JSON.parse(fs.readFileSync(file, 'utf8'));
            } catch (_) {
                return {};
            }
        }
        return {};
    }

    _save() {
        const data = {
            goalWeight: this.goalWeight,
            goalDeadline: this.goalDeadline,
            unit: this.unit,
            height: this.height,
            entries: this.entries
        };
        fs.writeFileSync(this._getDataPath(), JSON.stringify(data, null, 2));
    }

    _today() {
        return new Date().toISOString().split('T')[0];
    }

    _getTodayEntry() {
        const today = this._today();
        return this.entries.find(e => e.date === today) || null;
    }

    _getTodayWeight() {
        const entry = this._getTodayEntry();
        return entry ? entry.weight : null;
    }

    _getAllEntriesSorted() {
        return this.entries.slice().sort((a, b) => a.date.localeCompare(b.date));
    }

    _getWeights() {
        return this._getAllEntriesSorted().map(e => e.weight).filter(w => w !== undefined);
    }

    _progressBar(current, goal, width = 20) {
        if (!goal) return '⚠️  Goal not set';
        const weights = this._getWeights();
        if (!weights.length) return 'No data';
        const start = weights[0];
        if (current >= start) {
            const ratio = Math.max(0, Math.min(1, (current - goal) / (start - goal)));
            const filled = Math.floor(ratio * width);
            return `[${'█'.repeat(filled)}${'░'.repeat(width - filled)}] ${(ratio * 100).toFixed(1)}%`;
        } else {
            const ratio = Math.max(0, Math.min(1, (start - current) / (start - goal)));
            const filled = Math.floor(ratio * width);
            return `[${'█'.repeat(filled)}${'░'.repeat(width - filled)}] ${(ratio * 100).toFixed(1)}%`;
        }
    }

    _ask(prompt) {
        return new Promise(resolve => this.rl.question(prompt, resolve));
    }

    async _askFloat(prompt) {
        while (true) {
            const ans = await this._ask(prompt);
            const num = parseFloat(ans.trim());
            if (!isNaN(num)) return num;
            console.log(c('❌ Please enter a number.', colors.red));
        }
    }

    async _askString(prompt) {
        return (await this._ask(prompt)).trim();
    }

    async _askConfirm(prompt) {
        const ans = await this._ask(prompt + ' (yes/no): ');
        return ans.trim().toLowerCase() === 'yes';
    }

    // ─── Core Actions ─────────────────────────────────────────────────────

    async addEntry(weight, notes = '') {
        if (weight <= 0) {
            console.log(c('❌ Weight must be positive!', colors.red));
            return;
        }
        const today = this._today();
        const existing = this._getTodayEntry();
        if (existing) {
            existing.weight = weight;
            existing.notes = notes;
        } else {
            this.entries.push({ date: today, weight, notes });
        }
        this._save();
        console.log(c(`✅ Weight logged: ${weight.toFixed(1)} ${this.unit}`, colors.green));
    }

    showToday() {
        const weight = this._getTodayWeight();
        if (weight === null) {
            console.log(c('No weight logged today.', colors.yellow));
            return;
        }
        const diff = weight - this.goalWeight;
        const status = diff > 0 ? 'above' : 'below';
        const color = diff > 0 ? colors.red : colors.green;
        console.log('\n' + c('═'.repeat(50), colors.dim));
        console.log(c('⚖️ TODAY\'S WEIGHT', colors.bright + colors.cyan));
        console.log(c('═'.repeat(50), colors.dim));
        console.log(`  Weight: ${c(weight.toFixed(1) + ' ' + this.unit, color)}`);
        console.log(`  Goal:   ${this.goalWeight.toFixed(1)} ${this.unit}`);
        console.log(`  You are ${c(Math.abs(diff).toFixed(1) + ' ' + this.unit + ' ' + status, color)} your goal`);
        console.log(`  Progress: ${this._progressBar(weight, this.goalWeight)}`);
        console.log(c('═'.repeat(50), colors.dim));
    }

    showChart(days = 30) {
        const entries = this._getAllEntriesSorted();
        if (!entries.length) {
            console.log(c('No data to chart.', colors.yellow));
            return;
        }
        let data = entries;
        if (data.length > days) data = data.slice(-days);
        const weights = data.map(e => e.weight);
        const dates = data.map(e => e.date.slice(5, 10));
        const minW = Math.min(...weights);
        const maxW = Math.max(...weights);
        const range = maxW - minW;
        if (range === 0) {
            console.log(c('Weight is constant. No variation to chart.', colors.yellow));
            return;
        }
        const height = 10;
        const chartWidth = 40;
        const norm = weights.map(w => Math.floor((w - minW) / range * (height - 1)));
        const lines = [];
        for (let row = height - 1; row >= 0; row--) {
            let line = '';
            for (let i = 0; i < norm.length; i++) {
                if (norm[i] >= row) {
                    if (i > 0 && norm[i-1] >= row) line += '─';
                    else line += '┌';
                } else {
                    line += ' ';
                }
            }
            lines.push(line);
        }
        const step = Math.max(1, Math.floor(dates.length / 8));
        let xAxis = ' ';
        let lastPos = 0;
        for (let i = 0; i < dates.length; i += step) {
            const label = dates[i];
            const pos = i;
            if (pos > lastPos) xAxis += ' '.repeat(pos - lastPos);
            xAxis += label;
            lastPos = pos;
        }
        if (lastPos < dates.length - 1) xAxis += ' '.repeat(dates.length - 1 - lastPos);
        console.log(`\n${c(`📈 Weight Chart (last ${data.length} days)`, colors.bright + colors.cyan)}`);
        console.log(lines.join('\n'));
        console.log(xAxis);
        console.log(`Min: ${minW.toFixed(1)} ${this.unit}  Max: ${maxW.toFixed(1)} ${this.unit}`);
    }

    showStats() {
        if (!this.entries.length) {
            console.log(c('📭 No data yet. Start tracking!', colors.yellow));
            return;
        }
        const weights = this._getWeights();
        if (!weights.length) {
            console.log(c('No weight data available.', colors.yellow));
            return;
        }
        const total = weights.length;
        const avg = weights.reduce((a, b) => a + b, 0) / total;
        const minW = Math.min(...weights);
        const maxW = Math.max(...weights);
        const last = weights[weights.length - 1];
        const first = weights[0];
        const change = last - first;
        const trend = change > 0 ? 'up' : 'down';
        // BMI
        const heightM = this.height / 100;
        const bmi = heightM > 0 ? last / (heightM * heightM) : 0;
        const bmiCategory = this._bmiCategory(bmi);
        console.log('\n📊 STATISTICS');
        console.log(c('─'.repeat(30), colors.dim));
        console.log(`  Total Entries:  ${total}`);
        console.log(`  Current Weight: ${c(last.toFixed(1) + ' ' + this.unit, colors.green)}`);
        console.log(`  Goal Weight:    ${this.goalWeight.toFixed(1)} ${this.unit}`);
        console.log(`  Average:        ${avg.toFixed(1)} ${this.unit}`);
        console.log(`  Minimum:        ${minW.toFixed(1)} ${this.unit}`);
        console.log(`  Maximum:        ${maxW.toFixed(1)} ${this.unit}`);
        console.log(`  Change:         ${change > 0 ? '+' : ''}${change.toFixed(1)} ${this.unit} (${trend})`);
        console.log(`  BMI:            ${bmi.toFixed(1)} (${bmiCategory})`);
    }

    _bmiCategory(bmi) {
        if (bmi < 18.5) return 'Underweight';
        if (bmi < 25) return 'Normal';
        if (bmi < 30) return 'Overweight';
        return 'Obese';
    }

    async setGoal(weight, deadline) {
        if (weight <= 0) {
            console.log(c('❌ Goal weight must be positive!', colors.red));
            return;
        }
        this.goalWeight = weight;
        if (deadline) {
            if (!/^\d{4}-\d{2}-\d{2}$/.test(deadline)) {
                console.log(c('⚠️  Invalid date format. Use YYYY-MM-DD. Keeping current deadline.', colors.yellow));
                return;
            }
            this.goalDeadline = deadline;
        } else {
            this.goalDeadline = this._defaultDeadline();
        }
        this._save();
        console.log(c(`✅ Goal set to ${weight.toFixed(1)} ${this.unit} by ${this.goalDeadline}`, colors.green));
    }

    async setHeight(height) {
        if (height <= 0) {
            console.log(c('❌ Height must be positive!', colors.red));
            return;
        }
        this.height = height;
        this._save();
        console.log(c(`✅ Height set to ${height.toFixed(1)} cm`, colors.green));
    }

    async setUnit(unit) {
        if (!['kg', 'lbs'].includes(unit)) {
            console.log(c('❌ Unit must be "kg" or "lbs"', colors.red));
            return;
        }
        this.unit = unit;
        this._save();
        console.log(c(`✅ Unit set to ${unit}`, colors.green));
    }

    async clearData() {
        if (!await this._askConfirm('⚠️  Delete ALL data? This cannot be undone!')) return;
        this.entries = [];
        this.goalWeight = CONFIG.defaultGoal;
        this.goalDeadline = this._defaultDeadline();
        this._save();
        console.log(c('🗑️  All data cleared.', colors.yellow));
    }

    // ─── Menu ─────────────────────────────────────────────────────────────

    async _showMenu() {
        const todayWeight = this._getTodayWeight();
        const progress = this._progressBar(todayWeight || 0, this.goalWeight);
        console.log('\n' + c('═'.repeat(50), colors.cyan));
        console.log(c('⚖️ WEIGHT TRACKER', colors.bright + colors.cyan));
        console.log(c('═'.repeat(50), colors.cyan));
        console.log(`  Today: ${todayWeight !== null ? todayWeight.toFixed(1) : '—'} ${this.unit} / ${this.goalWeight.toFixed(1)} ${this.unit}`);
        console.log(`  Goal deadline: ${this.goalDeadline}`);
        console.log(`  Progress: ${progress}`);
        console.log(c('─'.repeat(50), colors.dim));
        console.log('  1. ⚖️ Log weight today');
        console.log('  2. 📊 Today\'s progress');
        console.log('  3. 📈 Show weight chart');
        console.log('  4. 📊 Statistics');
        console.log('  5. 🎯 Set goal weight & deadline');
        console.log('  6. 📏 Set height');
        console.log('  7. 🔄 Set unit (kg/lbs)');
        console.log('  8. 🗑️  Clear all data');
        console.log('  0. 🚪 Exit');
        console.log(c('═'.repeat(50), colors.cyan));
    }

    async run() {
        console.clear();
        console.log(c('\n⚖️ Weight Tracker – Goal‑Based Fitness Manager', colors.bright + colors.cyan));
        console.log(c('Track your weight, reach your goals!', colors.dim));

        while (true) {
            await this._showMenu();
            const choice = await this._ask('Your choice: ');
            switch (choice.trim()) {
                case '1': {
                    const weight = await this._askFloat(`Weight (${this.unit}): `);
                    const notes = await this._askString('Notes (optional): ');
                    await this.addEntry(weight, notes);
                    break;
                }
                case '2': this.showToday(); break;
                case '3': this.showChart(); break;
                case '4': this.showStats(); break;
                case '5': {
                    const weight = await this._askFloat(`Goal weight (${this.unit}): `);
                    const deadline = await this._askString('Deadline (YYYY-MM-DD) [leave empty for default]: ');
                    await this.setGoal(weight, deadline || null);
                    break;
                }
                case '6': {
                    const height = await this._askFloat('Height (cm): ');
                    await this.setHeight(height);
                    break;
                }
                case '7': {
                    const unit = await this._askString('Unit (kg/lbs): ');
                    await this.setUnit(unit.toLowerCase());
                    break;
                }
                case '8': await this.clearData(); break;
                case '0':
                    console.log(c('👋 Stay fit! Goodbye!', colors.cyan));
                    this.rl.close();
                    return;
                default:
                    console.log(c('❌ Invalid choice.', colors.red));
            }
            if (choice !== '0') {
                console.log('\nPress Enter to continue...');
                await this._ask('');
            }
        }
    }
}

// ─── Main ────────────────────────────────────────────────────────────────────

const main = async () => {
    try {
        const app = new WeightTracker();
        await app.run();
    } catch (e) {
        console.error(c(`❌ Unexpected error: ${e.message}`, colors.red));
        process.exit(1);
    }
};

main();
