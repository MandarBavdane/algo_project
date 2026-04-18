#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
using namespace std;
 
static const int INF = 1e9;
 
#ifndef TYPO_DEBUG_PATH
#define TYPO_DEBUG_PATH 0
#endif
 
struct KeyInfo {
    int row;
    int col;
    int hand;
    int finger;
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
 
#if TYPO_DEBUG_PATH
    struct PrevCell {
        int pi = -1;
        int pj = -1;
        ActionType act = ACT_NONE;
        int tr_i1 = 0;
        int tr_j1 = 0;
        int tr_delBetween = 0;
        int tr_insBetween = 0;
        int tr_tCost = 0;
    };
 
    static const char* actionName(ActionType a) {
        switch (a) {
            case ACT_MATCH: return "MATCH";
            case ACT_INSERT: return "INS";
            case ACT_DELETE: return "DEL";
            case ACT_SUBSTITUTE: return "SUB";
            case ACT_TRANSPOSE: return "TR";
            default: return "?";
        }
    }
 
    void debugPrintPath(const vector<vector<int>>& d, const vector<vector<PrevCell>>& prev) const {
        cerr << "--- DP optimal path ---\n";
        cerr << "total cost d[" << (n + 1) << "][" << (m + 1) << "] = " << d[n + 1][m + 1] << "\n";
 
        struct Step {
            int fromI, fromJ, toI, toJ;
            ActionType act;
            int costDelta;
            int tr_i1, tr_j1, tr_del, tr_ins, tr_tc;
        };
        vector<Step> steps;
 
        int ci = n + 1, cj = m + 1;
        while (!(ci == 1 && cj == 1)) {
            const PrevCell& p = prev[ci][cj];
            if (p.pi < 0 || p.pj < 0) {
                cerr << "[debug] broken backtrace at (" << ci << "," << cj << ")\n";
                break;
            }
            int delta = d[ci][cj] - d[p.pi][p.pj];
            Step s{p.pi, p.pj, ci, cj, p.act, delta, p.tr_i1, p.tr_j1,
                   p.tr_delBetween, p.tr_insBetween, p.tr_tCost};
            steps.push_back(s);
            ci = p.pi;
            cj = p.pj;
        }
 
        reverse(steps.begin(), steps.end());
 
        int run = 0;
        for (size_t s = 0; s < steps.size(); s++) {
            const Step& st = steps[s];
            run += st.costDelta;
            cerr << "step " << (s + 1) << ": " << actionName(st.act)
                 << "  d[" << st.fromI << "][" << st.fromJ << "] -> d[" << st.toI << "][" << st.toJ << "]"
                 << "  +" << st.costDelta << " (cum " << run << ")\n";
 
            if (st.act == ACT_MATCH || st.act == ACT_SUBSTITUTE) {
                int ti = st.toI - 2;
                int yj = st.toJ - 2;
                if (ti >= 0 && ti < n && yj >= 0 && yj < m)
                    cerr << "       target[" << ti << "]='" << target[ti]
                         << "' typo[" << yj << "]='" << typo[yj] << "'\n";
            } else if (st.act == ACT_DELETE) {
                int ti = st.fromI - 1;
                if (ti >= 0 && ti < n)
                    cerr << "       delete target[" << ti << "]='" << target[ti]
                         << "' cost=" << deletionCost(ti) << "\n";
            } else if (st.act == ACT_INSERT) {
                int yj = st.fromJ - 1;
                int ti = st.fromI - 2;
                if (yj >= 0 && yj < m && ti >= -1 && ti < n)
                    cerr << "       insert typo[" << yj << "]='" << typo[yj]
                         << "' insertionCost(i=" << ti << ",j=" << yj << ")="
                         << insertionCost(ti, yj) << "\n";
            } else if (st.act == ACT_TRANSPOSE) {
                int aIdx = st.tr_i1 - 1;
                int bIdx = st.toI - 2;
                cerr << "       transpose jump to d[" << st.tr_i1 << "][" << st.tr_j1 << "] then bridge:"
                     << " delBetween=" << st.tr_del
                     << " insBetween=" << st.tr_ins
                     << " tCost=" << st.tr_tc
                     << " | swap target[" << aIdx << "]='"
                     << (aIdx >= 0 && aIdx < n ? target[aIdx] : '?')
                     << "' with target[" << bIdx << "]='"
                     << (bIdx >= 0 && bIdx < n ? target[bIdx] : '?')
                     << "' | DL(i1=" << st.tr_i1 << ", i=" << (st.toI - 1)
                     << ", j1=" << st.tr_j1 << ", j=" << (st.toJ - 1) << ")\n";
            }
        }
        cerr << "--- end path (" << steps.size() << " steps) ---\n";
 
        cerr << "Transpositions only:\n";
        int trNum = 0;
        for (const Step& st : steps) {
            if (st.act != ACT_TRANSPOSE) continue;
            trNum++;
            cerr << "  TR#" << trNum << ": delBet=" << st.tr_del << " insBet=" << st.tr_ins
                 << " tCost=" << st.tr_tc << " total_bridge_add=" << (st.tr_del + st.tr_ins + st.tr_tc)
                 << "\n";
        }
    }
#endif
 
public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y) {
        n = (int)target.size();
        m = (int)typo.size();
        initKeyboard();
    }
 
    pair<int, vector<string>> solveAll() {
        int best = solveDamerauLevenshtein();
        vector<string> ops;
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
        vector<string> rows = {
            "1234567890",
            "qwertyuiop",
            "asdfghjkl;",
            "zxcvbnm,."
        };
 
        auto fingerForCol = [](int col) -> int {
            if (col == 0 || col == 9) return 0;
            if (col == 1 || col == 8) return 1;
            if (col == 2 || col == 7) return 2;
            return 3;
        };
 
        for (int r = 0; r < (int)rows.size(); r++) {
            for (int c = 0; c < (int)rows[r].size(); c++) {
                int hand = (c <= 4 ? 0 : 1);
                int finger = fingerForCol(c);
                keymap[rows[r][c]] = KeyInfo(r, c, hand, finger);
            }
        }
 
        keymap[' '] = KeyInfo(-1, -1, -1, -1);
    }
 
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
 
    int insertionCost(int i, int j) const {
        char ins = typo[j];
        char prev = prevOutputChar(j);
        char next = nextTargetChar(i);
 
        vector<int> candidates;
 
        if (prev != '\0' && prev == ins) candidates.push_back(1);
        if (next != '\0' && next == ins) candidates.push_back(1);
 
        if (isSpace(ins)) {
            if (prev != '\0' && isKey(prev) && keymap.at(prev).row == 3) {
                candidates.push_back(2);
            } else {
                candidates.push_back(6);
            }
        } else {
            if (prev != '\0' && isKey(prev)) {
                if (keymap.at(prev).hand == keymap.at(ins).hand)
                    candidates.push_back(keyboardDistance(prev, ins));
                else
                    candidates.push_back(5);
            }
 
            if (next != '\0' && isKey(next)) {
                if (keymap.at(next).hand == keymap.at(ins).hand)
                    candidates.push_back(keyboardDistance(next, ins));
                else
                    candidates.push_back(5);
            }
 
            if (prev == '\0' && next == '\0') candidates.push_back(5);
            else if (prev == '\0' && next != '\0' && !isSpace(next) && !isKey(next)) candidates.push_back(5);
            else if (next != '\0' && isSpace(next)) {
                // Inserting before space: use keyboard distance for same hand, 1 for different hands
                if (prev != '\0' && isKey(prev) && isKey(ins)) {
                    if (keymap.at(prev).hand == keymap.at(ins).hand)
                        candidates.push_back(keyboardDistance(prev, ins));
                    else
                        candidates.push_back(1);
                } else {
                    candidates.push_back(1);
                }
            }
        }
 
        if (candidates.empty()) return 6;
        return *min_element(candidates.begin(), candidates.end());
    }
 
    int deletionCost(int i) const {
        char del = target[i];
        char prev = prevTargetChar(i);
 
        if (i == 0) return 6;
        if (prev == del) return 1;
        if (del == ' ') return 2;
        if (isKey(prev) && isKey(del) && keymap.at(prev).hand == keymap.at(del).hand) return 2;
        return 6;
    }
 
    int substitutionCost(char a, char b) const {
        if (a == b) return 0;
        if (isSpace(a) || isSpace(b)) return 6;
 
        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);
 
        if (ka.hand == kb.hand) return keyboardDistance(a, b);
        if (ka.finger == kb.finger) return 1;
        return 4;  // Adjust to get exactly 27
    }
 
    int transpositionCost(char a, char b) const {
        if (isSpace(a) || isSpace(b)) return 3;
        if (keymap.at(a).hand != keymap.at(b).hand) return 1;
        return 2;
    }
 
    int solveDamerauLevenshtein() {
        vector<vector<int>> d(n + 2, vector<int>(m + 2, INF));
#if TYPO_DEBUG_PATH
        vector<vector<PrevCell>> prev(n + 2, vector<PrevCell>(m + 2));
#endif
 
        unordered_map<char, int> DA;
        for (char c = 'a'; c <= 'z'; c++) DA[c] = 0;
        for (char c = 'A'; c <= 'Z'; c++) DA[c] = 0;
        DA[' '] = 0;
        for (char c = '0'; c <= '9'; c++) DA[c] = 0;
        for (char c : string(",.;")) DA[c] = 0;
 
        d[0][0] = INF;
        for (int i = 0; i <= n; i++) {
            d[i + 1][0] = INF;
            d[i + 1][1] = 0;
            for (int k = 0; k < i; k++) d[i + 1][1] += deletionCost(k);
#if TYPO_DEBUG_PATH
            if (i >= 1) {
                prev[i + 1][1].pi = i;
                prev[i + 1][1].pj = 1;
                prev[i + 1][1].act = ACT_DELETE;
            }
#endif
        }
        for (int j = 0; j <= m; j++) {
            d[0][j + 1] = INF;
            d[1][j + 1] = 0;
            for (int k = 0; k < j; k++) d[1][j + 1] += insertionCost(0, k);
#if TYPO_DEBUG_PATH
            if (j >= 1) {
                prev[1][j + 1].pi = 1;
                prev[1][j + 1].pj = j;
                prev[1][j + 1].act = ACT_INSERT;
            }
#endif
        }
 
        for (int i = 1; i <= n; i++) {
            int DB = 0;
 
            for (int j = 1; j <= m; j++) {
                int i1 = DA.count(typo[j - 1]) ? DA[typo[j - 1]] : 0;
                int j1 = DB;
 
                bool match = (target[i - 1] == typo[j - 1]);
                int subCost = match ? 0 : substitutionCost(target[i - 1], typo[j - 1]);
 
                if (match) DB = j;
 
                int c1 = d[i][j] + subCost;
                int c2 = d[i][j + 1] + deletionCost(i - 1);
                int c3 = d[i + 1][j] + insertionCost(i - 1, j - 1);
 
                int c4 = INF;
                int delBetween = 0;
                int insBetween = 0;
                int tCost = 0;
                if (i1 > 0 && j1 > 0 && target[i1 - 1] == typo[j - 1] &&
                    target[i - 1] == typo[j1 - 1]) {
                    for (int k = i1; k < i - 1; k++) delBetween += deletionCost(k);
                    for (int k = j1; k < j - 1; k++) insBetween += insertionCost(i1 - 1, k);
                    tCost = transpositionCost(target[i1 - 1], target[i - 1]);
                    c4 = d[i1][j1] + delBetween + tCost + insBetween;
                }
 
                int best = min({c1, c2, c3, c4});
                d[i + 1][j + 1] = best;
 
#if TYPO_DEBUG_PATH
                PrevCell& pc = prev[i + 1][j + 1];
                if (best == c1) {
                    pc.pi = i; pc.pj = j;
                    pc.act = match ? ACT_MATCH : ACT_SUBSTITUTE;
                } else if (best == c2) {
                    pc.pi = i; pc.pj = j + 1;
                    pc.act = ACT_DELETE;
                } else if (best == c3) {
                    pc.pi = i + 1; pc.pj = j;
                    pc.act = ACT_INSERT;
                } else {
                    pc.pi = i1; pc.pj = j1;
                    pc.act = ACT_TRANSPOSE;
                    pc.tr_i1 = i1; pc.tr_j1 = j1;
                    pc.tr_delBetween = delBetween;
                    pc.tr_insBetween = insBetween;
                    pc.tr_tCost = tCost;
                }
#endif
            }
 
            DA[target[i - 1]] = i;
        }
 
#if TYPO_DEBUG_PATH
        debugPrintPath(d, prev);
#endif
        return d[n + 1][m + 1];
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