.syntax unified
.cpu cortex-m33
.thumb

.global nova_arch_call_on_stack
.type nova_arch_call_on_stack, %function
.thumb_func
nova_arch_call_on_stack:
    push {r4-r11, lr}
    mov r4, sp
    sub sp, #4
    mov r5, r1
    mov r6, r2
    mov sp, r0
    mov r0, r6
    blx r5
    mov sp, r4
    pop {r4-r11, pc}
.size nova_arch_call_on_stack, .-nova_arch_call_on_stack
