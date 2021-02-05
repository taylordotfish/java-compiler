/*
 * Copyright (C) 2019 taylor.fish <contact@taylor.fish>
 *
 * This file is part of java-compiler.
 *
 * java-compiler is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * java-compiler is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with java-compiler. If not, see <https://www.gnu.org/licenses/>.
 */

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
