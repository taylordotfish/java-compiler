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

.globl printf
.globl fish_java_x64_print_char
.globl fish_java_x64_print_int
.globl fish_java_x64_println_void
.globl fish_java_x64_println_char
.globl fish_java_x64_println_int

.text
fish_java_x64_print_char:
    push %rbp
    mov %rsp, %rbp
    sub $0x8, %rsp
    lea fmt_string_char(%rip), %rdi
    mov 16(%rbp), %rsi
    call printf
    add $0x8, %rsp
    pop %rbp
    ret

fish_java_x64_print_int:
    push %rbp
    mov %rsp, %rbp
    sub $0x8, %rsp
    lea fmt_string_int(%rip), %rdi
    mov 16(%rbp), %rsi
    call printf
    add $0x8, %rsp
    pop %rbp
    ret

fish_java_x64_println_void:
    push %rbp
    mov %rsp, %rbp
    sub $0x8, %rsp
    lea fmt_string_nl(%rip), %rdi
    mov 16(%rbp), %rsi
    call printf
    add $0x8, %rsp
    pop %rbp
    ret

fish_java_x64_println_char:
    push %rbp
    mov %rsp, %rbp
    sub $0x8, %rsp
    lea fmt_string_char_nl(%rip), %rdi
    mov 16(%rbp), %rsi
    call printf
    add $0x8, %rsp
    pop %rbp
    ret

fish_java_x64_println_int:
    push %rbp
    mov %rsp, %rbp
    sub $0x8, %rsp
    lea fmt_string_int_nl(%rip), %rdi
    mov 16(%rbp), %rsi
    call printf
    add $0x8, %rsp
    pop %rbp
    ret

.data
fmt_string_char:
    .string "%c"

fmt_string_int:
    .string "%ld"

fmt_string_nl:
    .string "\n"

fmt_string_char_nl:
    .string "%c\n"

fmt_string_int_nl:
    .string "%ld\n"
