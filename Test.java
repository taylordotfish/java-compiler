class Test {
    public static void main(String[] args) {
        fizzBuzz(1, 100);
    }

    public static void fizzBuzz(int start, int end) {
        for (int i = start; i <= end; ++i) {
            if (i % 3 == 0) {
                System.out.print("Fizz");
            }
            if (i % 5 == 0) {
                System.out.print("Buzz");
            }
            if (i % 3 != 0 && i % 5 != 0) {
                System.out.print(i);
            }
            System.out.println();
        }
    }
}
