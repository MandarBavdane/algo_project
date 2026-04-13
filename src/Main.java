public class Main {
    public static void main(String[] args) {
        String target = "the";
        String typo = "teh";

        DP solver = new DP(target, typo);
        int result = solver.compute();

        System.out.println(result);
    }
}