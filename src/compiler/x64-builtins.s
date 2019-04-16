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
