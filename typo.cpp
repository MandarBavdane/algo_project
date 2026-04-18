#include <bits/stdc++.h>
using namespace std;

static const int INF = 1e9;

struct KeyInfo {
    int row;
    int col;
    int hand;   // 0 = left, 1 = right, -1 = special/space
    int finger; // finger group, used for same-finger opposite-hand rule
    bool exists;
    KeyInfo() : row(-1), col(-1), hand(-1), finger(-1), exists(false) {}
    KeyInfo(int r, int c, int h, int f) : row(r), col(c), hand(h), finger(f), exists(true) {}
};

enum ActionType {
    ACT_NONE = 0,
    ACT_MATCH,
    ACT_INSERT,
    ACT_DELETE,
    ACT_SUBSTITUTE,
    ACT_TRANSPOSE
};

struct Decision {
    ActionType type;
    char ch; // for insert/substitute
    Decision(ActionType t = ACT_NONE, char c = '\0') : type(t), ch(c) {}
};

class TypoSolver {
private:
    string target;
    string typo;
    int n, m;

    vector<vector<int>> memo;
    vector<vector<bool>> seen;
    vector<vector<Decision>> choice;

    unordered_map<char, KeyInfo> keymap;

public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y) {
        n = (int)target.size();
        m = (int)typo.size();
        memo.assign(n + 1, vector<int>(m + 1, INF));
        seen.assign(n + 1, vector<bool>(m + 1, false));
        choice.assign(n + 1, vector<Decision>(m + 1));
        initKeyboard();
    }

    pair<int, vector<string>> solveAll() {
        int best = solve(0, 0);
        vector<string> ops = reconstruct();
        return {best, ops};
    }

private:
    static bool isSpace(char c) {
        return c == ' ';
    }

    bool isKey(char c) const {
        return !isSpace(c) && keymap.count(c) && keymap.at(c).exists;
    }

    int keyboardDistance(char a, char b) const {
        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);
        return max(abs(ka.row - kb.row), abs(ka.col - kb.col));
    }

    void initKeyboard() {
        // Standard QWERTY layout specified in prompt.
        vector<string> rows = {
            "1234567890",
            "qwertyuiop",
            "asdfghjkl;",
            "zxcvbnm,."
        };

        // Approximate touch-typing finger groups consistent across rows.
        // Left hand columns: 0..4, right hand columns: 5..9
        // Finger groups:
        // left pinky=0, left ring=1, left middle=2, left index=3
        // right index=4, right middle=5, right ring=6, right pinky=7
        auto fingerForCol = [](int col) -> int {
            if (col == 0) return 0;
            if (col == 1) return 1;
            if (col == 2) return 2;
            if (col == 3 || col == 4) return 3;
            if (col == 5 || col == 6) return 4;
            if (col == 7) return 5;
            if (col == 8) return 6;
            return 7; // col == 9
        };

        for (int r = 0; r < (int)rows.size(); r++) {
            for (int c = 0; c < (int)rows[r].size(); c++) {
                int hand = (c <= 4 ? 0 : 1);
                int finger = fingerForCol(c);
                keymap[rows[r][c]] = KeyInfo(r, c, hand, finger);
            }
        }

        // Space handled specially, but keep an entry.
        keymap[' '] = KeyInfo(-1, -1, -1, -1);
    }

    // Build the current output prefix from state (i,j): typo[0..j-1].
    char prevOutputChar(int j) const {
        if (j <= 0) return '\0';
        return typo[j - 1];
    }

    char nextTargetChar(int i) const {
        if (i >= n) return '\0';
        return target[i];
    }

    char prevTargetChar(int i) const {
        if (i <= 0) return '\0';
        return target[i - 1];
    }

    // Insertion cost for inserting ins between previous output char and next target char.
    int insertionCost(int i, int j) const {
        char ins = typo[j];
        char prev = prevOutputChar(j);
        char next = nextTargetChar(i);

        vector<int> candidates;

        // Repeated character.
        if (prev != '\0' && prev == ins) candidates.push_back(1);
        if (next != '\0' && next == ins) candidates.push_back(1);

        if (isSpace(ins)) {
            // Space after key on bottom row.
            if (prev != '\0' && isKey(prev) && keymap.at(prev).row == 3) {
                candidates.push_back(2);
            }
            // Space after something else.
            if (prev != '\0') {
                candidates.push_back(6);
            }
            // Beginning/end or no previous char: fall back to generic space behavior.
            if (prev == '\0') {
                candidates.push_back(6);
            }
        } else {
            // Non-space before or after a space.
            if (prev == ' ' || next == ' ') {
                candidates.push_back(6);
            }

            // Before or after another key on same hand => d(k1, k2)
            if (prev != '\0' && isKey(prev)) {
                if (keymap.at(prev).hand == keymap.at(ins).hand) {
                    candidates.push_back(keyboardDistance(prev, ins));
                } else {
                    // Before or after a key on opposite hand => 5
                    candidates.push_back(5);
                }
            }

            if (next != '\0' && isKey(next)) {
                if (keymap.at(next).hand == keymap.at(ins).hand) {
                    candidates.push_back(keyboardDistance(next, ins));
                } else {
                    candidates.push_back(5);
                }
            }

            // If there is no neighboring evidence, use conservative fallback.
            if (prev == '\0' && next == '\0') {
                candidates.push_back(5);
            } else if (prev == '\0' && next != '\0' && !isSpace(next) && !isKey(next)) {
                candidates.push_back(5);
            }
        }

        if (candidates.empty()) {
            // Safe fallback.
            return 6;
        }
        return *min_element(candidates.begin(), candidates.end());
    }

    int deletionCost(int i) const {
        char del = target[i];
        char prev = prevTargetChar(i);

        // First character in string.
        if (i == 0) return 6;

        // Repeated character.
        if (prev == del) return 1;

        // Space.
        if (del == ' ') return 3;

        // Character after another key on same hand.
        if (isKey(prev) && isKey(del) && keymap.at(prev).hand == keymap.at(del).hand) {
            return 2;
        }

        // Character after space or key on different hand.
        return 6;
    }

    int substitutionCost(char a, char b) const {
        if (a == b) return 0;

        if (isSpace(a) || isSpace(b)) {
            return 6;
        }

        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);

        // Key for another on same hand.
        if (ka.hand == kb.hand) {
            return keyboardDistance(a, b);
        }

        // Key for another on same finger, other hand.
        if (ka.finger == kb.finger) {
            return 1;
        }

        // Key for another on different finger, other hand.
        return 5;
    }

    int transpositionCost(char a, char b) const {
        if (isSpace(a) || isSpace(b)) return 3;
        if (keymap.at(a).hand != keymap.at(b).hand) return 1;
        return 2;
    }

    void consider(int& best, Decision& bestDec, int cost, ActionType type, char ch = '\0') {
        // Deterministic tiebreak order:
        // MATCH, TRANSPOSE, SUBSTITUTE, INSERT, DELETE
        auto rank = [](ActionType t) -> int {
            switch (t) {
                case ACT_MATCH: return 0;
                case ACT_TRANSPOSE: return 1;
                case ACT_SUBSTITUTE: return 2;
                case ACT_INSERT: return 3;
                case ACT_DELETE: return 4;
                default: return 5;
            }
        };

        if (cost < best || (cost == best && rank(type) < rank(bestDec.type))) {
            best = cost;
            bestDec = Decision(type, ch);
        }
    }

    int solve(int i, int j) {
        if (seen[i][j]) return memo[i][j];
        seen[i][j] = true;

        if (i == n && j == m) {
            memo[i][j] = 0;
            choice[i][j] = Decision(ACT_NONE, '\0');
            return 0;
        }

        int best = INF;
        Decision bestDec(ACT_NONE, '\0');

        // Match
        if (i < n && j < m && target[i] == typo[j]) {
            int v = solve(i + 1, j + 1);
            consider(best, bestDec, v, ACT_MATCH);
        }

        // Insert
        if (j < m) {
            int c = insertionCost(i, j);
            int v = c + solve(i, j + 1);
            consider(best, bestDec, v, ACT_INSERT, typo[j]);
        }

        // Delete
        if (i < n) {
            int c = deletionCost(i);
            int v = c + solve(i + 1, j);
            consider(best, bestDec, v, ACT_DELETE);
        }

        // Substitute
        if (i < n && j < m) {
            int c = substitutionCost(target[i], typo[j]);
            int v = c + solve(i + 1, j + 1);
            consider(best, bestDec, v, ACT_SUBSTITUTE, typo[j]);
        }

        // Transpose
        if (i + 1 < n && j + 1 < m &&
            target[i] == typo[j + 1] &&
            target[i + 1] == typo[j]) {
            int c = transpositionCost(target[i], target[i + 1]);
            int v = c + solve(i + 2, j + 2);
            consider(best, bestDec, v, ACT_TRANSPOSE);
        }

        memo[i][j] = best;
        choice[i][j] = bestDec;
        return best;
    }

    vector<string> reconstruct() const {
        vector<string> ops;
        int i = 0, j = 0;
        int pos = 0;

        while (!(i == n && j == m)) {
            const Decision& d = choice[i][j];

            if (d.type == ACT_MATCH) {
                i++;
                j++;
                pos++;
            } else if (d.type == ACT_INSERT) {
                ops.push_back(string("Insert ") + d.ch + " before " + to_string(pos));
                j++;
                pos++;
            } else if (d.type == ACT_DELETE) {
                ops.push_back("Delete " + to_string(pos));
                i++;
            } else if (d.type == ACT_SUBSTITUTE) {
                ops.push_back(string("Substitute ") + d.ch + " at " + to_string(pos));
                i++;
                j++;
                pos++;
            } else if (d.type == ACT_TRANSPOSE) {
                ops.push_back("Transpose " + to_string(pos) + "-" + to_string(pos + 1));
                i += 2;
                j += 2;
                pos += 2;
            } else {
                // Defensive break in case of unexpected state.
                break;
            }
        }

        return ops;
    }
};

static vector<pair<string, string>> readInputFile(const string& filename) {
    ifstream fin(filename);
    if (!fin) {
        throw runtime_error("Could not open input.txt");
    }

    string line;
    getline(fin, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    int cases = stoi(line);

    vector<pair<string, string>> tests;
    tests.reserve(cases);

    for (int tc = 0; tc < cases; tc++) {
        string target, typo;

        // Skip blank lines before target if present.
        while (getline(fin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) {
                target = line;
                break;
            }
        }

        if (!getline(fin, typo)) {
            throw runtime_error("Missing typo string in input.");
        }
        if (!typo.empty() && typo.back() == '\r') typo.pop_back();

        tests.push_back({target, typo});
    }

    return tests;
}

static void writeOutputFile(
    const string& filename,
    const vector<pair<int, vector<string>>>& results
) {
    ofstream fout(filename);
    if (!fout) {
        throw runtime_error("Could not open output.txt");
    }

    for (size_t t = 0; t < results.size(); t++) {
        fout << results[t].first << "\n";
        for (const string& op : results[t].second) {
            fout << op << "\n";
        }
        if (t + 1 < results.size()) {
            fout << "\n";
        }
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    try {
        vector<pair<string, string>> tests = readInputFile("input.txt");
        vector<pair<int, vector<string>>> results;
        results.reserve(tests.size());

        for (const auto& test : tests) {
            TypoSolver solver(test.first, test.second);
            results.push_back(solver.solveAll());
        }

        writeOutputFile("output.txt", results);
    } catch (const exception& e) {
        // For grading environments, avoid extra stdout noise.
        // Write a minimal error to stderr only.
        cerr << e.what() << "\n";
        return 1;
    }

    return 0;
}