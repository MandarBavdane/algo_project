public class DP {

    private String target;
    private String typo;
    private int[][] dp;
    private int[][] parent;

    public static final int DELETE = 1;
    public static final int INSERT = 2;
    public static final int SUBSTITUTE = 3;
    public static final int TRANSPOSE = 4;

    public DP(String target, String typo) {
        this.target = target;
        this.typo = typo;
        this.dp = new int[target.length() + 1][typo.length() + 1];
        this.parent = new int[target.length() + 1][typo.length() + 1];
    }

    public int compute() {
        int n = target.length();
        int m = typo.length();

        dp[0][0] = 0;
        for (int i = 1; i <= n; i++) {
            dp[i][0] = dp[i - 1][0] + getCostDel(target.charAt(i - 1));
            parent[i][0] = DELETE;
        }
        for (int j = 0; j <= m; j++) {
            dp[0][j] = dp[0][j - 1] + getCostIns(target.charAt(j - 1));
            parent[0][j] = INSERT;
        }

        for (int i = 0; i <= n; i++) {
            for (int j = 0; j <= m; j++) {
                dp[i][j] = Integer.MAX_VALUE;

                int insCost = getCostIns(typo.charAt(j-1));
                if (dp[i][j-1] + insCost < dp[i][j]) {
                    dp[i][j] = dp[i][j-1] + insCost;
                    parent[i][j] = INSERT;
                }

                int delCost = getCostDel(target.charAt(i-1));
                if (dp[i-1][j] + delCost < dp[i][j]) {
                    dp[i][j] = dp[i-1][j] + delCost;
                    parent[i][j] = DELETE;
                }

                int subCost = getCostSub(target.charAt(i - 1), target.charAt(j - 1));
                if (dp[i-1][j-1] + subCost < dp[i][j]) {
                    dp[i][j] = dp[i-1][j-1] + subCost;
                    parent[i][j] = SUBSTITUTE;
                }

                if (i > 1 && j > 1 && target.charAt(i-2) == typo.charAt(j-1) && target.charAt(i-1) == typo.charAt(j-2)) {
                    int transCost = getCostTrans(target.charAt(i-2), target.charAt(i-1));
                    if (dp[i-2][j-2] + transCost < dp[i][j]) {
                        dp[i][j] = dp[i-2][j-2] + transCost;
                        parent[i][j] = TRANSPOSE;
                    }
                }
            }
        }
        return dp[n][m];
    }
}

private int getCostIns(char a) {
    return 1;
}

private int getCostDel(char a) {
    return 1;
}

private int getCostSub(char a, char b) {
    return (a == b) ? 0 : 1;
}

private int getCostTrans(char a, char b) {
    return 1;
}