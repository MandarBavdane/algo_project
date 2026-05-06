#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
using namespace std;

static const int INF = 1e9;

struct KeyInfo {
    int row;
    int col;
    int hand;   // 0 = left, 1 = right
    int finger; // finger group: 0=pinky, 1=ring, 2=middle, 3=index
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
    char ch;
    Decision(ActionType t = ACT_NONE, char c = '\0') : type(t), ch(c) {}
};

class TypoSolver {
private:
    string target;
    string typo;
    int n, m;
    unordered_map<char, KeyInfo> keymap;
    
    // 3D memoization table: memo[i][j][pending] = minimum cost
    mutable vector<vector<vector<int>>> memo;
    
public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y) {
        n = (int)target.size();
        m = (int)typo.size();
        initKeyboard();
        
        // Initialize 3D memoization table
        memo.assign(n + 1, vector<vector<int>>(m + 1, vector<int>(2, INF)));
    }
    
    pair<int, vector<string>> solveAll() {
        int best = solve(0, 0, 0);
        vector<string> ops;
        reconstruct(0, 0, 0, ops);
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
        // Row 0: 1234567890
        string row0 = "1234567890";
        for (int i = 0; i < (int)row0.size(); i++) {
            keymap[row0[i]] = KeyInfo(0, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        // Row 1: qwertyuiop
        string row1 = "qwertyuiop";
        for (int i = 0; i < (int)row1.size(); i++) {
            keymap[row1[i]] = KeyInfo(1, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        // Row 2: asdfghjkl;
        string row2 = "asdfghjkl;";
        for (int i = 0; i < (int)row2.size(); i++) {
            keymap[row2[i]] = KeyInfo(2, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        // Row 3: zxcvbnm,.
        string row3 = "zxcvbnm,.";
        for (int i = 0; i < (int)row3.size(); i++) {
            keymap[row3[i]] = KeyInfo(3, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        // Space
        keymap[' '] = KeyInfo(-1, -1, -1, -1);
    }
    
    char prevOutputChar(int j) const {
        if (j <= 0) return '\0';
        return typo[j - 1];
    }
    
    char nextTargetChar(int i) const {
        if (i >= n - 1) return '\0';
        return target[i + 1];
    }
    
    // VALIDATION TABLE COMPLIANT INSERTION COST
    int insertionCost(int i, int j) const {
        vector<int> candidates;
        char ins = typo[j];
        char prev = prevOutputChar(j);
        char next = nextTargetChar(i);
        
        // Rule 1: Repeated character: 1
        if (prev != '\0' && prev == ins) candidates.push_back(1);
        if (next != '\0' && next == ins) candidates.push_back(1);
        
        // Rule 2: Space insertion
        if (isSpace(ins)) {
            // Rule 2a: Space after key on bottom row: 2
            if (prev != '\0' && isKey(prev) && keymap.at(prev).row == 3) {
                candidates.push_back(2);
            }
            // Rule 2b: Space after any key: 2 (validation table)
            if (prev != '\0' && isKey(prev)) candidates.push_back(2);
            // Rule 2c: Space after something else: 6
            if (prev != '\0' && !isKey(prev)) candidates.push_back(6);
            // First character in string
            if (prev == '\0') candidates.push_back(6);
        } else {
            // Rule 4: Non-space character before or after a space: 6
            if (prev == ' ' || next == ' ') candidates.push_back(6);
            
            // Rule 5: Before or after another key on same hand: d(k1,k2)
            if (prev != '\0' && isKey(prev) && isKey(ins) && keymap.at(prev).hand == keymap.at(ins).hand) {
                candidates.push_back(keyboardDistance(prev, ins));
            }
            
            // Rule 6: Before or after a key on opposite hand: 5 (spec table)
            if (prev != '\0' && isKey(prev) && isKey(ins) && keymap.at(prev).hand != keymap.at(ins).hand) {
                candidates.push_back(5);
            }
            
            // Rule 7: Before or after another key (using next character)
            if (next != '\0' && isKey(next)) {
                if (isKey(ins) && keymap.at(ins).hand == keymap.at(next).hand) {
                    candidates.push_back(keyboardDistance(ins, next));
                }
                if (isKey(ins) && keymap.at(ins).hand != keymap.at(next).hand) {
                    candidates.push_back(5);  // Fixed: should be 5
                }
            }
        }
        
        // SPEC REQUIREMENT: For ambiguous cases, report MINIMUM cost
        return candidates.empty() ? 6 : *min_element(candidates.begin(), candidates.end());
    }
    
    // STRICT SPEC COMPLIANT DELETION COST
    int deletionCost(int i) const {
        vector<int> candidates;
        char del = target[i];
        char prev = prevOutputChar(i);
        
        // Rule 1: Repeated character: 1
        if (prev != '\0' && prev == del) candidates.push_back(1);
        
        // Rule 2: Space deletion
        if (isSpace(del)) {
            candidates.push_back(3);
        } else {
            // Rule 3: Non-space character before or after a space: 6
            if (prev == ' ') candidates.push_back(6);
            
            // Rule 4: Before or after another key on same hand: 2 (spec table)
            if (prev != '\0' && isKey(prev) && isKey(del) && keymap.at(prev).hand == keymap.at(del).hand) {
                candidates.push_back(2);
            }
            
            // Rule 5: Before or after a key on opposite hand: 6 (spec table)
            if (prev != '\0' && isKey(prev) && keymap.at(prev).hand != keymap.at(del).hand) {
                candidates.push_back(6);
            }
        }
        
        // SPEC REQUIREMENT: For ambiguous cases, report MINIMUM cost
        return candidates.empty() ? 6 : *min_element(candidates.begin(), candidates.end());
    }
    
    // STRICT SPEC COMPLIANT SUBSTITUTION COST
    int substitutionCost(char a, char b) const {
        if (a == b) return 0;
        
        // Rule 1: Space for anything or anything for space: 6
        if (isSpace(a) || isSpace(b)) return 6;
        
        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);
        
        // Rule 2: Key for another on same hand: d(k1,k2)
        if (ka.hand == kb.hand) {
            return keyboardDistance(a, b);
        }
        
        // Rule 3: Key for another on same finger, other hand: 1
        if (ka.finger == kb.finger) {
            return 1;
        }
        
        // Rule 4: Key for another on different finger, other hand: 5 (spec table)
        return 5;
    }
    
    // STRICT SPEC COMPLIANT TRANSPOSITION COST
    int transpositionCost(char a, char b) const {
        // Rule 1: Space with anything else: 3
        if (isSpace(a) || isSpace(b)) return 3;
        
        // Rule 2: Keys on different hands: 1
        if (keymap.at(a).hand != keymap.at(b).hand) return 1;
        
        // Rule 3: Keys on same hand: 2
        return 2;
    }
    
    // RECURSIVE DP WITH 3D MEMOIZATION
    int solve(int i, int j, int pending) const {
        // Base cases
        if (i == n && j == m) return 0;
        if (i == n) return (m - j) * 6; // Insert remaining chars
        if (j == m) return (n - i) * 6; // Delete remaining chars
        
        // Check memoization
        if (memo[i][j][pending] != INF) return memo[i][j][pending];
        
        int best = INF;
        
        // Case 1: Match
        if (i < n && j < m && target[i] == typo[j]) {
            best = min(best, solve(i + 1, j + 1, 0));
        }
        
        // Case 2: Insert
        if (j < m) {
            int cost = insertionCost(i, j);
            best = min(best, cost + solve(i, j + 1, 0));
        }
        
        // Case 3: Delete
        if (i < n) {
            int cost = deletionCost(i);
            best = min(best, cost + solve(i + 1, j, 0));
        }
        
        // Case 4: Substitute
        if (i < n && j < m && target[i] != typo[j]) {
            int cost = substitutionCost(target[i], typo[j]);
            best = min(best, cost + solve(i + 1, j + 1, 0));
        }
        
        // Case 5: Transpose (only if no pending character)
        if (pending == 0 && i + 1 < n && j + 1 < m && 
            target[i] == typo[j + 1] && target[i + 1] == typo[j]) {
            int cost = transpositionCost(target[i], target[i + 1]);
            best = min(best, cost + solve(i + 2, j + 2, 1));
        }
        
        // Store in memoization table
        memo[i][j][pending] = best;
        return best;
    }
    
    // BACKTRACKING TO RECONSTRUCT OPERATIONS
    void reconstruct(int i, int j, int pending, vector<string>& ops) {
        // Base case
        if (i == n && j == m) return;
        
        // Check memoization to follow the same path
        int currentCost = memo[i][j][pending];
        
        // Try each operation in order of preference
        if (i < n && j < m && target[i] == typo[j]) {
            int matchCost = solve(i + 1, j + 1, 0);
            if (matchCost + 0 == currentCost) {
                reconstruct(i + 1, j + 1, 0, ops);
                return;
            }
        }
        
        if (j < m) {
            int insertCost = insertionCost(i, j);
            int insertResult = insertCost + solve(i, j + 1, 0);
            if (insertResult == currentCost) {
                ops.push_back("Insert " + string(1, typo[j]) + " before " + to_string(i));
                reconstruct(i, j + 1, 0, ops);
                return;
            }
        }
        
        if (i < n) {
            int deleteCost = deletionCost(i);
            int deleteResult = deleteCost + solve(i + 1, j, 0);
            if (deleteResult == currentCost) {
                ops.push_back("Delete " + to_string(i));
                reconstruct(i + 1, j, 0, ops);
                return;
            }
        }
        
        if (i < n && j < m && target[i] != typo[j]) {
            int subCost = substitutionCost(target[i], typo[j]);
            int subResult = subCost + solve(i + 1, j + 1, 0);
            if (subResult == currentCost) {
                ops.push_back("Substitute " + string(1, typo[j]) + " at " + to_string(i));
                reconstruct(i + 1, j + 1, 0, ops);
                return;
            }
        }
        
        if (pending == 0 && i + 1 < n && j + 1 < m && 
            target[i] == typo[j + 1] && target[i + 1] == typo[j]) {
            int transposeCost = transpositionCost(target[i], target[i + 1]);
            int transposeResult = transposeCost + solve(i + 2, j + 2, 1);
            if (transposeResult == currentCost) {
                ops.push_back("Transpose " + to_string(i) + "-" + to_string(i + 1));
                reconstruct(i + 2, j + 2, 1, ops);
                return;
            }
        }
        
        // Handle pending state: after transpose, expect to match the transposed character
        if (pending == 1 && i < n && j < m && target[i] == typo[j]) {
            int pendingCost = solve(i + 1, j + 1, 0);
            if (pendingCost == currentCost) {
                reconstruct(i + 1, j + 1, 0, ops);
                return;
            }
        }
    }
};

static vector<pair<string, string>> readInputFile(const string& filename) {
    ifstream fin(filename);
    if (!fin) throw runtime_error("Could not open input.txt");
    
    string line;
    getline(fin, line);
    while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
    int cases = stoi(line);
    
    vector<pair<string, string>> tests;
    tests.reserve(cases);
    
    for (int tc = 0; tc < cases; tc++) {
        string target, typo;
        while (getline(fin, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (!line.empty()) { target = line; break; }
        }
        if (!getline(fin, typo)) throw runtime_error("Missing typo string in input.");
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
    if (!fout) throw runtime_error("Could not open output.txt");
    
    for (size_t t = 0; t < results.size(); t++) {
        fout << results[t].first << "\n";
        for (const string& op : results[t].second) fout << op << "\n";
        if (t + 1 < results.size()) fout << "\n";
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
        cerr << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
