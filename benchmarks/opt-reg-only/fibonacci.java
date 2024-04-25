public class fibonacci {
    public fibonacci() {}
    public static int test() {
        return fibonacci.fibonacciCalc(25);
    }
    public static int fibonacciCalc(int n) {
        if (n <= 1) {
            return n;
        } else {
            return fibonacci.fibonacciCalc(n - 1) + fibonacci.fibonacciCalc(n - 2);
        }
    }
}