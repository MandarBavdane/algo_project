#include <bits/stdc++.h>
using namespace std;

static const int INF = 1000000000;

class TypoSolver {
private:
    enum Kind {
        MATCH,
        INSERT,
        DELETE_OP,
        SUBSTITUTE,
        TRANSPOSE,
        END
    };

    struct Choice {
        Kind kind = END;
        int advanceI = 0;
        int advanceJ = 0;
        int emitTransposeCount = 0; 
    };

    string target, typo;
    int n, m;
    vector<vector<int>> memo;
    vector<vector<char>> seen;
    vector<vector<Choice>> choice;
    unordered_map<char, pair<int,int>> pos;

public:
    TypoSolver(const string& t, const string& y) : target(t), typo(y), n((int)t.size()), m((int)y.size()) {
        memo.assign(n + 1, vector<int>(m + 1, INF));
        seen.assign(n + 1, vector<char>(m + 1, 0));
        choice.assign(n + 1, vector<Choice>(m + 1));
        buildKeyboardPositions();
    }

    pair<int, vector<string>> solveAll() {
        int best = solve(0, 0);
        vector<string> ops = reconstruct();
        return {best, ops};
    }

private:
    void buildKeyboardPositions() {
        vector<string> rows = {
            "1234567890",
            "qwertyuiop",
            "asdfghjkl;",
            "zxcvbnm,."
        };
        for (int r = 0; r < (int)rows.size(); ++r) {
            for (int c = 0; c < (int)rows[r].size(); ++c) {
                pos[rows[r][c]] = {r, c};
            }
        }
    }

    int solve(int i, int j) {
        if (seen[i][j]) return memo[i][j];
        seen[i][j] = 1;

        if (i == n && j == m) {
            memo[i][j] = 0;
            choice[i][j] = {END, 0, 0, 0};
            return 0;
        }

        int best = INF;
        Choice bestChoice{END, 0, 0, 0};

        auto relax = [&](int cost, Choice candChoice) {
            if (cost < best) {
                best = cost;
                bestChoice = candChoice;
            }
        };

        if (i < n && j < m && target[i] == typo[j]) {
            int tail = solve(i + 1, j + 1);
            if (tail < INF) relax(tail, {MATCH, 1, 1, 0});
        }

        if (j < m) {
            int c = insertionCost(i, j, typo[j]);
            int tail = solve(i, j + 1);
            if (tail < INF && c + tail < INF) relax(c + tail, {INSERT, 0, 1, 0});
        }

        if (i < n) {
            int c = deletionCost(i, target[i]);
            int tail = solve(i + 1, j);
            if (tail < INF && c + tail < INF) relax(c + tail, {DELETE_OP, 1, 0, 0});
        }

        if (i < n && j < m && target[i] != typo[j]) {
            int c = substitutionCost(target[i], typo[j]);
            int tail = solve(i + 1, j + 1);
            if (tail < INF && c + tail < INF) relax(c + tail, {SUBSTITUTE, 1, 1, 0});
        }

        if (i + 1 < n && j + 1 < m && target[i] == typo[j + 1] && target[i + 1] == typo[j]) {
            int c = transposeCost(target[i], target[i + 1]);
            int tail = solve(i + 2, j + 2);
            if (tail < INF && c + tail < INF) relax(c + tail, {TRANSPOSE, 2, 2, 1});
        }

        if (i + 2 < n && j + 2 < m &&
            target[i + 1] == typo[j] &&
            target[i + 2] == typo[j + 1] &&
            target[i]     == typo[j + 2]) {
            int c1 = transposeCost(target[i], target[i + 1]);
            int c2 = transposeCost(target[i], target[i + 2]);
            int tail = solve(i + 3, j + 3);
            if (tail < INF && c1 + c2 + tail < INF) {
                relax(c1 + c2 + tail, {TRANSPOSE, 3, 3, 2});
            }
        }

        memo[i][j] = best;
        choice[i][j] = bestChoice;
        return best;
    }

    vector<string> reconstruct() const {
        vector<string> ops;
        int i = 0, j = 0;
        while (!(i == n && j == m)) {
            const Choice& ch = choice[i][j];
            switch (ch.kind) {
                case MATCH:
                    i += 1;
                    j += 1;
                    break;
                case INSERT:
                    ops.push_back(string("Insert ") + typo[j] + " before " + to_string(j));
                    j += 1;
                    break;
                case DELETE_OP:
                    ops.push_back("Delete " + to_string(j));
                    i += 1;
                    break;
                case SUBSTITUTE:
                    ops.push_back(string("Substitute ") + typo[j] + " at " + to_string(j));
                    i += 1;
                    j += 1;
                    break;
                case TRANSPOSE:
                    if (ch.emitTransposeCount == 1) {
                        ops.push_back("Transpose " + to_string(j) + "-" + to_string(j + 1));
                    } else if (ch.emitTransposeCount == 2) {
                        ops.push_back("Transpose " + to_string(j) + "-" + to_string(j + 1));
                        ops.push_back("Transpose " + to_string(j + 1) + "-" + to_string(j + 2));
                    } else {
                        throw runtime_error("Invalid transpose reconstruction state");
                    }
                    i += ch.advanceI;
                    j += ch.advanceJ;
                    break;
                case END:
                    if (i == n && j == m) return ops;
                    throw runtime_error("Missing reconstruction choice");
            }
        }
        return ops;
    }

    int insertionCost(int i, int j, char inserted) const {
        int best = INF;

        if (i > 0) {
            best = min(best, insertionNearNeighbor(target[i - 1], inserted));
        }
        if (j > 0) {
            best = min(best, insertionNearNeighbor(typo[j - 1], inserted));
        }
        if (i < n) {
            best = min(best, insertionNearNeighbor(target[i], inserted));
        }

        return best == INF ? 6 : best;
    }

    int insertionNearNeighbor(char neighbor, char inserted) const {
        if (inserted == neighbor) return 1;

        if (inserted == ' ') {
            if (isBottomRowKey(neighbor)) return 2;
            return 6;
        }

        if (neighbor == ' ') return 6;

        if (sameHand(inserted, neighbor)) return keyboardDistance(inserted, neighbor);
        return 5;
    }

    int deletionCost(int i, char deleted) const {
        if (i == 0) return 6;
        return deletionAgainstPrevNeighbor(deleted, target[i - 1]);
    }

    int deletionAgainstPrevNeighbor(char deleted, char prev) const {
        if (deleted == prev) return 1;
        if (deleted == ' ') return 3;
        if (prev != ' ' && sameHand(deleted, prev)) return 2;
        return 6;
    }

    int substitutionCost(char from, char to) const {
        if (from == ' ' || to == ' ') return 6;
        if (sameHand(from, to)) return keyboardDistance(from, to);
        if (sameFingerOtherHand(from, to)) return 1;
        return 5;
    }

    int transposeCost(char a, char b) const {
        if (a == ' ' || b == ' ') return 3;
        if (!sameHand(a, b)) return 1;
        return 2;
    }

    int keyboardDistance(char a, char b) const {
        auto ita = pos.find(a), itb = pos.find(b);
        if (ita == pos.end() || itb == pos.end()) return 5;
        return max(abs(ita->second.first - itb->second.first), abs(ita->second.second - itb->second.second));
    }

    bool isBottomRowKey(char c) const {
        auto it = pos.find(c);
        return it != pos.end() && it->second.first == 3;
    }

    bool sameHand(char a, char b) const {
        auto ha = handOf(a), hb = handOf(b);
        return ha != -1 && hb != -1 && ha == hb;
    }

    bool sameFingerOtherHand(char a, char b) const {
        int ha = handOf(a), hb = handOf(b);
        if (ha == -1 || hb == -1 || ha == hb) return false;
        int fa = fingerGroup(a), fb = fingerGroup(b);
        return fa != -1 && fa == fb;
    }

    int handOf(char c) const {
        if (c == ' ') return -1;
        auto it = pos.find(c);
        if (it == pos.end()) return -1;
        return (it->second.second <= 4) ? 0 : 1;
    }

    int fingerGroup(char c) const {
        if (c == ' ') return -1;
        auto it = pos.find(c);
        if (it == pos.end()) return -1;
        int col = it->second.second;
        return min(col, 9 - col);
    }
};

static vector<pair<string,string>> readInput(const string& path) {
    ifstream fin(path);
    if (!fin) throw runtime_error("Could not open input.txt");

    vector<string> lines;
    string line;
    while (getline(fin, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
    }

    if (lines.empty()) throw runtime_error("input.txt is empty");

    int idx = 0;
    while (idx < (int)lines.size() && lines[idx].empty()) ++idx;
    if (idx >= (int)lines.size()) throw runtime_error("Missing test count in input.txt");

    int t = stoi(lines[idx++]);
    vector<pair<string,string>> tests;
    tests.reserve(t);

    for (int caseNo = 0; caseNo < t; ++caseNo) {
        while (idx < (int)lines.size() && lines[idx].empty()) ++idx;
        if (idx >= (int)lines.size()) throw runtime_error("Missing target string for case " + to_string(caseNo + 1));
        string target = lines[idx++];
        if (idx >= (int)lines.size()) throw runtime_error("Missing typo string for case " + to_string(caseNo + 1));
        string typo = lines[idx++];
        tests.push_back({target, typo});
    }

    return tests;
}

static void writeOutput(const string& path, const vector<pair<int, vector<string>>>& results) {
    ofstream fout(path);
    if (!fout) throw runtime_error("Could not open output.txt");

    for (size_t i = 0; i < results.size(); ++i) {
        fout << results[i].first << "\n";
        for (const string& op : results[i].second) fout << op << "\n";
        if (i + 1 < results.size()) fout << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    try {
        vector<pair<string,string>> tests = readInput("input.txt");
        vector<pair<int, vector<string>>> results;
        results.reserve(tests.size());

        for (const auto& tc : tests) {
            TypoSolver solver(tc.first, tc.second);
            results.push_back(solver.solveAll());
        }

        writeOutput("output.txt", results);
    } catch (const exception& e) {
        cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}