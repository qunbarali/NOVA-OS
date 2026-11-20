.syntax unified
.cpu cortex-m33
.thumb
.global nova_arch_context_switch
.type nova_arch_context_switch, %function
.thumb_func
nova_arch_context_switch:
    mov r12, r2
    mrs r3, control
    tst r3, #2
    bne 1f

    /* Scheduler saves on MSP; tasks execute on PSP; IRQ handlers use MSP. */
    push {r4-r11, lr}
    mov r2, sp
    str r2, [r0]
    mov r4, r12
    msr msp, r4
    msr psp, r1
    orr r3, r3, #2
    msr control, r3
    isb
    pop {r4-r11, pc}

1:
    /* Save task PSP and restore the scheduler's saved MSP. */
    push {r4-r11, lr}
    mov r2, sp
    str r2, [r0]
    msr msp, r1
    bic r3, r3, #2
    msr control, r3
    isb
    pop {r4-r11, pc}
.size nova_arch_context_switch, .-nova_arch_context_switch
