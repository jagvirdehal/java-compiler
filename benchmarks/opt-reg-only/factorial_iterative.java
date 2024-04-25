public class factorial_iterative {
    public factorial_iterative() {}
    public static int test() {
        int n = 600000;
        int result = 1;
        while (n > 0) {
            result = result * n;
            n = n - 1;
        }
        return 123;
    }
}