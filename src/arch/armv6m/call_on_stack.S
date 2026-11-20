.syntax unified
.cpu cortex-m0plus
.thumb
.global nova_arch_context_switch
.type nova_arch_context_switch, %function
.thumb_func
nova_arch_context_switch:
    mov r12, r2
    mrs r3, control
    movs r2, #2
    tst r3, r2
    bne 1f

    /* Scheduler saves on MSP; tasks execute on PSP; IRQ handlers use MSP. */
    push {r4-r7, lr}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}
    mov r4, r12
    mov r2, sp
    str r2, [r0]
    msr msp, r4
    msr psp, r1
    movs r2, #2
    orrs r3, r2
    msr control, r3
    isb
    pop {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7, pc}

1:
    /* Save task PSP and restore the scheduler's saved MSP. */
    push {r4-r7, lr}
    mov r4, r8
    mov r5, r9
    mov r6, r10
    mov r7, r11
    push {r4-r7}
    mov r2, sp
    str r2, [r0]
    msr msp, r1
    movs r2, #2
    bics r3, r2
    msr control, r3
    isb
    pop {r4-r7}
    mov r8, r4
    mov r9, r5
    mov r10, r6
    mov r11, r7
    pop {r4-r7, pc}
.size nova_arch_context_switch, .-nova_arch_context_switch
