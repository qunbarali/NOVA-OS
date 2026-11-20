.syntax unified
.cpu cortex-m0plus
.thumb
.global HardFault_Handler
.type HardFault_Handler, %function
.thumb_func
HardFault_Handler:
    mov r1, lr
    movs r2, #4
    tst r1, r2
    beq 1f
    mrs r0, psp
    b 2f
1:
    mrs r0, msp
2:
    b nova_arch_fault_entry
.size HardFault_Handler, .-HardFault_Handler
