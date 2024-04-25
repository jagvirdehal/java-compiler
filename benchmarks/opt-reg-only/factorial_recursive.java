public class factorial_recursive {
    public factorial_recursive() {}
    public static int test() {
        return factorial_recursive.fact(50000);
    }
    public static int fact(int n) {
        if (n == 0) {
            return 1;
        } else {
            return n * factorial_recursive.fact(n - 1);
        }
	}
}