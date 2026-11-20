.syntax unified
.cpu cortex-m0plus
.thumb

.global nova_arch_call_on_stack
.type nova_arch_call_on_stack, %function
.thumb_func
nova_arch_call_on_stack:
    push {r4-r7, lr}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}
    mov r4, sp
    sub sp, #4
    mov r5, r1
    mov r6, r2
    mov sp, r0
    mov r0, r6
    blx r5
    mov sp, r4
    pop {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7, pc}
.size nova_arch_call_on_stack, .-nova_arch_call_on_stack
