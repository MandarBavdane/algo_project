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
 
struct PrevCell {
    int pi = -1;
    int pj = -1;
    ActionType act = ACT_NONE;
};
 
class TypoSolver {
private:
    string target;
    string typo;
    int n, m;
    unordered_map<char, KeyInfo> keymap;
    
public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y) {
        n = (int)target.size();
        m = (int)typo.size();
        initKeyboard();
    }
    
    pair<int, vector<string>> solveAll() {
        vector<vector<PrevCell>> prev;
        int best = solveDamerauLevenshtein(prev);
        vector<string> ops = reconstructOperations(prev);
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
        string row0 = "1234567890";
        for (int i = 0; i < (int)row0.size(); i++) {
            keymap[row0[i]] = KeyInfo(0, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        string row1 = "qwertyuiop";
        for (int i = 0; i < (int)row1.size(); i++) {
            keymap[row1[i]] = KeyInfo(1, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        string row2 = "asdfghjkl;";
        for (int i = 0; i < (int)row2.size(); i++) {
            keymap[row2[i]] = KeyInfo(2, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
        }
        
        string row3 = "zxcvbnm,.";
        for (int i = 0; i < (int)row3.size(); i++) {
            keymap[row3[i]] = KeyInfo(3, i, (i <= 4 ? 0 : 1), (i == 0 || i == 9 ? 0 : i == 1 || i == 8 ? 1 : i == 2 || i == 7 ? 2 : 3));
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
        vector<int> candidates;
        
        char ins = typo[j];
        char prev = prevOutputChar(j);
        char next = nextTargetChar(i);
        
        if (prev != '\0' && prev == ins) candidates.push_back(1);
        if (next != '\0' && next == ins) candidates.push_back(1);
        
        if (isSpace(ins)) {
            if (prev != '\0' && isKey(prev) && keymap.at(prev).row == 3) {
                candidates.push_back(2);
            }
            if (prev != '\0' && isKey(prev)) candidates.push_back(2);
            if (prev != '\0' && !isKey(prev)) candidates.push_back(6);
            if (prev == '\0') candidates.push_back(6);
        } else {
            if (prev == ' ' || next == ' ') candidates.push_back(6);
            
            if (prev != '\0' && isKey(prev) && isKey(ins) && keymap.at(prev).hand == keymap.at(ins).hand) {
                candidates.push_back(keyboardDistance(prev, ins));
            }
            
            if (prev != '\0' && isKey(prev) && isKey(ins) && keymap.at(prev).hand != keymap.at(ins).hand) {
                candidates.push_back(3);
            }
            
            if (next != '\0' && isKey(next)) {
                if (isKey(ins) && keymap.at(ins).hand == keymap.at(next).hand) {
                    candidates.push_back(keyboardDistance(ins, next));
                }
                if (isKey(ins) && keymap.at(ins).hand != keymap.at(next).hand) {
                    candidates.push_back(3);
                }
            }
        }
        
        return candidates.empty() ? 6 : *min_element(candidates.begin(), candidates.end());
    }
    
    int deletionCost(int i) const {
        char del = target[i];
        char prev = prevTargetChar(i);
        
        if (prev == del) return 1;
        if (del == ' ') return 3;
        if (isKey(prev) && isKey(del) && keymap.at(prev).hand == keymap.at(del).hand) {
            return 2;
        }
        return 6;
    }
    
    int substitutionCost(char a, char b) const {
        if (a == b) return 0;
        if (isSpace(a) || isSpace(b)) return 6;
        
        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);
        
        if (ka.hand == kb.hand) {
            return keyboardDistance(a, b);
        }
        if (ka.finger == kb.finger) {
            return 1;
        }
        return 5;
    }
    
    int transpositionCost(char a, char b) const {
        if (isSpace(a) || isSpace(b)) return 3;
        if (keymap.at(a).hand != keymap.at(b).hand) return 1;
        return 2;
    }
    
    int solveDamerauLevenshtein(vector<vector<PrevCell>>& prev) {
        vector<vector<int>> d(n + 2, vector<int>(m + 2, INF));
        prev.assign(n + 2, vector<PrevCell>(m + 2));
        
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
            if (i >= 1) {
                prev[i + 1][1].pi = i;
                prev[i + 1][1].pj = 1;
                prev[i + 1][1].act = ACT_DELETE;
            }
        }
        for (int j = 0; j <= m; j++) {
            d[0][j + 1] = INF;
            d[1][j + 1] = 0;
            for (int k = 0; k < j; k++) d[1][j + 1] += insertionCost(0, k);
            if (j >= 1) {
                prev[1][j + 1].pi = 1;
                prev[1][j + 1].pj = j;
                prev[1][j + 1].act = ACT_INSERT;
            }
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
                if (i1 > 0 && j1 > 0 && target[i1 - 1] == typo[j - 1] &&
                    target[i - 1] == typo[j1 - 1]) {
                    int delBetween = 0;
                    int insBetween = 0;
                    for (int k = i1; k < i - 1; k++) delBetween += deletionCost(k);
                    for (int k = j1; k < j - 1; k++) insBetween += insertionCost(i1 - 1, k);
                    int tCost = transpositionCost(target[i1 - 1], target[i - 1]);
                    c4 = d[i1][j1] + delBetween + tCost + insBetween;
                }
                
                int best = min({c1, c2, c3, c4});
                d[i + 1][j + 1] = best;
                
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
                }
            }
            
            DA[target[i - 1]] = i;
        }
        
        return d[n + 1][m + 1];
    }
    
    vector<string> reconstructOperations(const vector<vector<PrevCell>>& prev) {
        vector<string> operations;
        
        struct Step {
            int fromI, fromJ, toI, toJ;
            ActionType act;
        };
        vector<Step> steps;
        
        int ci = n + 1, cj = m + 1;
        while (!(ci == 1 && cj == 1)) {
            const PrevCell& p = prev[ci][cj];
            if (p.pi < 0 || p.pj < 0) break;
            
            Step s{p.pi, p.pj, ci, cj, p.act};
            steps.push_back(s);
            ci = p.pi;
            cj = p.pj;
        }
        
        reverse(steps.begin(), steps.end());
        
        int currentIndex = 0;
        for (const Step& st : steps) {
            if (st.act == ACT_DELETE) {
                operations.push_back("Delete " + to_string(currentIndex));
            } else if (st.act == ACT_INSERT) {
                int yj = st.fromJ - 1;
                if (yj >= 0 && yj < m) {
                    operations.push_back("Insert " + string(1, typo[yj]) + " before " + to_string(currentIndex));
                    currentIndex++;
                }
            } else if (st.act == ACT_SUBSTITUTE) {
                int yj = st.toJ - 2;
                if (yj >= 0 && yj < m) {
                    operations.push_back("Substitute " + string(1, typo[yj]) + " at " + to_string(currentIndex));
                }
                currentIndex++;
            } else if (st.act == ACT_TRANSPOSE) {
                operations.push_back("Transpose " + to_string(currentIndex) + "-" + to_string(currentIndex + 1));
                currentIndex += 2;
            } else if (st.act == ACT_MATCH) {
                currentIndex++;
            }
        }
        
        return operations;
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