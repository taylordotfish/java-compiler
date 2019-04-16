class Primes {
    public static void printPrimes(int start, int end) {
        System.out.print('P');
        System.out.print('r');
        System.out.print('i');
        System.out.print('m');
        System.out.print('e');
        System.out.print('s');
        System.out.print(' ');
        System.out.print('f');
        System.out.print('r');
        System.out.print('o');
        System.out.print('m');
        System.out.print(' ');
        System.out.print(start);
        System.out.print(' ');
        System.out.print('t');
        System.out.print('o');
        System.out.print(' ');
        System.out.print(end);
        System.out.println(':');

        for (int i = start; i <= end; ++i) {
            if (isPrime(i)) {
                System.out.println(i);
            }
        }
    }

    public static boolean isPrime(int x) {
        for (int i = 2; i < x >> 1; ++i) {
            if (isDivisible(x, i)) {
                return false;
            }
        }
        return x >= 2;
    }

    public static boolean isDivisible(int dividend, int divisor) {
        int i = divisor;
        for (; i < dividend; i += divisor);
        return i == dividend;
    }

    public static void main(String[] args) {
        printPrimes(1, 120);
    }
}
