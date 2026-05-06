#include <bits/stdc++.h>
using namespace std;

static const int INF = 1e9;

enum ActionType {
    ACT_NONE = 0,
    ACT_MATCH,
    ACT_INSERT,
    ACT_DELETE,
    ACT_SUBSTITUTE,
    ACT_TRANSPOSE
};

struct KeyInfo {
    int row, col, hand, finger;
    bool exists;
    KeyInfo() : row(-1), col(-1), hand(-1), finger(-1), exists(false) {}
    KeyInfo(int r, int c, int h, int f) : row(r), col(c), hand(h), finger(f), exists(true) {}
};

struct Parent {
    int pi, pj;
    ActionType type;
    int trans_i1, trans_j1;
    Parent() : pi(-1), pj(-1), type(ACT_NONE), trans_i1(-1), trans_j1(-1) {}
};

struct AbstractOp {
    ActionType type;
    int idx;
    int idx2;
    char ch;
    AbstractOp(ActionType t = ACT_NONE, int a = -1, int b = -1, char c = '\0')
        : type(t), idx(a), idx2(b), ch(c) {}
};

struct Node {
    char ch;
    int originalIndex;
    Node(char c = '\0', int idx = -1) : ch(c), originalIndex(idx) {}
};

class TypoSolver {
private:
    string target, typo;
    int n, m;
    unordered_map<char, KeyInfo> keymap;

public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y), n((int)t.size()), m((int)y.size()) {
        initKeyboard();
    }

    pair<int, vector<string>> solveAll() {
        vector<vector<int>> d;
        vector<vector<Parent>> parent;
        int best = solveDamerauLevenshtein(d, parent);
        vector<AbstractOp> abstractOps;
        emitAbstractOps(n + 1, m + 1, parent, abstractOps);
        vector<string> ops = materializeOperations(abstractOps);
        return {best, ops};
    }

private:
    static bool isSpace(char c) { return c == ' '; }

    bool isKey(char c) const {
        auto it = keymap.find(c);
        return !isSpace(c) && it != keymap.end() && it->second.exists;
    }

    int keyboardDistance(char a, char b) const {
        const KeyInfo& ka = keymap.at(a);
        const KeyInfo& kb = keymap.at(b);
        return max(abs(ka.row - kb.row), abs(ka.col - kb.col));
    }

    void initKeyboard() {
        auto fingerOf = [](int i, int lastIdx) {
            if (i == 0 || i == lastIdx) return 0;
            if (i == 1 || i == lastIdx - 1) return 1;
            if (i == 2 || i == lastIdx - 2) return 2;
            return 3;
        };

        string row0 = "1234567890";
        for (int i = 0; i < (int)row0.size(); ++i) keymap[row0[i]] = KeyInfo(0, i, (i <= 4 ? 0 : 1), fingerOf(i, 9));
        string row1 = "qwertyuiop";
        for (int i = 0; i < (int)row1.size(); ++i) keymap[row1[i]] = KeyInfo(1, i, (i <= 4 ? 0 : 1), fingerOf(i, 9));
        string row2 = "asdfghjkl;";
        for (int i = 0; i < (int)row2.size(); ++i) keymap[row2[i]] = KeyInfo(2, i, (i <= 4 ? 0 : 1), fingerOf(i, 9));
        string row3 = "zxcvbnm,.";
        for (int i = 0; i < (int)row3.size(); ++i) keymap[row3[i]] = KeyInfo(3, i, (i <= 4 ? 0 : 1), fingerOf(i, 8));
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
            if (prev != '\0' && isKey(prev) && keymap.at(prev).row == 3) candidates.push_back(2);
            if (prev != '\0' && isKey(prev)) candidates.push_back(2);
            if (prev != '\0' && !isKey(prev)) candidates.push_back(6);
            if (prev == '\0') candidates.push_back(6);
        } else {
            if (prev == ' ' || next == ' ') candidates.push_back(6);

            if (prev != '\0' && isKey(prev) && isKey(ins)) {
                if (keymap.at(prev).hand == keymap.at(ins).hand) candidates.push_back(keyboardDistance(prev, ins));
                else candidates.push_back(3);
            }

            if (next != '\0' && isKey(next) && isKey(ins)) {
                if (keymap.at(next).hand == keymap.at(ins).hand) candidates.push_back(keyboardDistance(ins, next));
                else candidates.push_back(3);
            }
        }

        return candidates.empty() ? 6 : *min_element(candidates.begin(), candidates.end());
    }

    int deletionCost(int i) const {
        char del = target[i];
        char prev = prevTargetChar(i);
        if (prev == del) return 1;
        if (del == ' ') return 3;
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
        return 5;
    }

    int transpositionCost(char a, char b) const {
        if (isSpace(a) || isSpace(b)) return 3;
        if (keymap.at(a).hand != keymap.at(b).hand) return 1;
        return 2;
    }

    void chooseBest(int candidateCost, const Parent& candidateParent,
                    int& bestCost, Parent& bestParent) const {
        if (candidateCost < bestCost) {
            bestCost = candidateCost;
            bestParent = candidateParent;
            return;
        }
        if (candidateCost > bestCost) return;

        auto rank = [](ActionType t) {
            switch (t) {
                case ACT_MATCH: return 0;
                case ACT_TRANSPOSE: return 1;
                case ACT_SUBSTITUTE: return 2;
                case ACT_DELETE: return 3;
                case ACT_INSERT: return 4;
                default: return 5;
            }
        };
        if (rank(candidateParent.type) < rank(bestParent.type)) bestParent = candidateParent;
    }

    int solveDamerauLevenshtein(vector<vector<int>>& d, vector<vector<Parent>>& parent) {
        d.assign(n + 2, vector<int>(m + 2, INF));
        parent.assign(n + 2, vector<Parent>(m + 2));

        unordered_map<char, int> DA;
        for (char c = 'a'; c <= 'z'; ++c) DA[c] = 0;
        DA[' '] = 0;
        for (char c = '0'; c <= '9'; ++c) DA[c] = 0;
        for (char c : string(",.;")) DA[c] = 0;

        d[0][0] = INF;
        d[1][1] = 0;
        parent[1][1] = Parent();

        for (int i = 0; i <= n; ++i) {
            d[i + 1][0] = INF;
            d[i + 1][1] = 0;
            for (int k = 0; k < i; ++k) d[i + 1][1] += deletionCost(k);
            if (i > 0) {
                parent[i + 1][1].pi = i;
                parent[i + 1][1].pj = 1;
                parent[i + 1][1].type = ACT_DELETE;
            }
        }
        for (int j = 0; j <= m; ++j) {
            d[0][j + 1] = INF;
            d[1][j + 1] = 0;
            for (int k = 0; k < j; ++k) d[1][j + 1] += insertionCost(0, k);
            if (j > 0) {
                parent[1][j + 1].pi = 1;
                parent[1][j + 1].pj = j;
                parent[1][j + 1].type = ACT_INSERT;
            }
        }

        for (int i = 1; i <= n; ++i) {
            int DB = 0;
            for (int j = 1; j <= m; ++j) {
                int i1 = DA.count(typo[j - 1]) ? DA[typo[j - 1]] : 0;
                int j1 = DB;
                bool match = (target[i - 1] == typo[j - 1]);
                int subCost = match ? 0 : substitutionCost(target[i - 1], typo[j - 1]);
                if (match) DB = j;

                int best = INF;
                Parent bestParent;

                Parent p1;
                p1.pi = i;
                p1.pj = j;
                p1.type = match ? ACT_MATCH : ACT_SUBSTITUTE;
                chooseBest(d[i][j] + subCost, p1, best, bestParent);

                Parent p2;
                p2.pi = i;
                p2.pj = j + 1;
                p2.type = ACT_DELETE;
                chooseBest(d[i][j + 1] + deletionCost(i - 1), p2, best, bestParent);

                Parent p3;
                p3.pi = i + 1;
                p3.pj = j;
                p3.type = ACT_INSERT;
                chooseBest(d[i + 1][j] + insertionCost(i - 1, j - 1), p3, best, bestParent);

                if (i1 > 0 && j1 > 0 && target[i1 - 1] == typo[j - 1] && target[i - 1] == typo[j1 - 1]) {
                    int delBetween = 0;
                    int insBetween = 0;
                    for (int k = i1; k < i - 1; ++k) delBetween += deletionCost(k);
                    for (int k = j1; k < j - 1; ++k) insBetween += insertionCost(i1 - 1, k);
                    int tCost = transpositionCost(target[i1 - 1], target[i - 1]);

                    Parent p4;
                    p4.pi = i1;
                    p4.pj = j1;
                    p4.type = ACT_TRANSPOSE;
                    p4.trans_i1 = i1;
                    p4.trans_j1 = j1;
                    chooseBest(d[i1][j1] + delBetween + tCost + insBetween, p4, best, bestParent);
                }

                d[i + 1][j + 1] = best;
                parent[i + 1][j + 1] = bestParent;
            }
            DA[target[i - 1]] = i;
        }

        return d[n + 1][m + 1];
    }

    void emitAbstractOps(int di, int dj, const vector<vector<Parent>>& parent, vector<AbstractOp>& out) const {
        if (di == 1 && dj == 1) return;
        const Parent& p = parent[di][dj];
        if (p.type == ACT_NONE) return;

        emitAbstractOps(p.pi, p.pj, parent, out);

        int i = di - 1;
        int j = dj - 1;

        switch (p.type) {
            case ACT_MATCH:
                break;
            case ACT_SUBSTITUTE:
                out.emplace_back(ACT_SUBSTITUTE, i - 1, -1, typo[j - 1]);
                break;
            case ACT_DELETE:
                out.emplace_back(ACT_DELETE, i - 1, -1, '\0');
                break;
            case ACT_INSERT:
                out.emplace_back(ACT_INSERT, i, -1, typo[j - 1]);
                break;
            case ACT_TRANSPOSE: {
                int i1 = p.trans_i1;
                int j1 = p.trans_j1;
                int firstIdx = i1 - 1;
                int secondIdx = i - 1;

                for (int k = firstIdx + 1; k <= secondIdx - 1; ++k) {
                    out.emplace_back(ACT_DELETE, k, -1, '\0');
                }
                out.emplace_back(ACT_TRANSPOSE, firstIdx, secondIdx, '\0');
                for (int k = j1; k <= j - 2; ++k) {
                    out.emplace_back(ACT_INSERT, firstIdx, -1, typo[k]);
                }
                break;
            }
            default:
                break;
        }
    }

    int findNodeByOriginal(const vector<Node>& cur, int originalIndex) const {
        for (int i = 0; i < (int)cur.size(); ++i) {
            if (cur[i].originalIndex == originalIndex) return i;
        }
        return -1;
    }

    vector<string> materializeOperations(const vector<AbstractOp>& abstractOps) const {
        vector<Node> cur;
        cur.reserve(target.size() + abstractOps.size() + 4);
        for (int i = 0; i < (int)target.size(); ++i) cur.emplace_back(target[i], i);

        vector<string> ans;
        ans.reserve(abstractOps.size());

        for (const auto& op : abstractOps) {
            if (op.type == ACT_INSERT) {
                int pos;
                if (op.idx >= n) pos = (int)cur.size();
                else {
                    pos = findNodeByOriginal(cur, op.idx);
                    if (pos < 0) pos = (int)cur.size();
                }
                ans.push_back(string("Insert ") + op.ch + " before " + to_string(pos));
                cur.insert(cur.begin() + pos, Node(op.ch, -1));
            } else if (op.type == ACT_DELETE) {
                int pos = findNodeByOriginal(cur, op.idx);
                if (pos < 0) continue;
                ans.push_back("Delete " + to_string(pos));
                cur.erase(cur.begin() + pos);
            } else if (op.type == ACT_SUBSTITUTE) {
                int pos = findNodeByOriginal(cur, op.idx);
                if (pos < 0) continue;
                ans.push_back(string("Substitute ") + op.ch + " at " + to_string(pos));
                cur[pos].ch = op.ch;
            } else if (op.type == ACT_TRANSPOSE) {
                int p1 = findNodeByOriginal(cur, op.idx);
                int p2 = findNodeByOriginal(cur, op.idx2);
                if (p1 < 0 || p2 < 0) continue;
                if (p1 > p2) swap(p1, p2);
                if (p2 != p1 + 1) continue;
                ans.push_back("Transpose " + to_string(p1) + "-" + to_string(p2));
                swap(cur[p1], cur[p2]);
            }
        }
        return ans;
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

    for (int tc = 0; tc < cases; ++tc) {
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

static void writeOutputFile(const string& filename, const vector<pair<int, vector<string>>>& results) {
    ofstream fout(filename);
    if (!fout) throw runtime_error("Could not open output.txt");
    for (size_t t = 0; t < results.size(); ++t) {
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