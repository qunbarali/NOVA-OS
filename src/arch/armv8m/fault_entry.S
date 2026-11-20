.syntax unified
.cpu cortex-m33
.thumb
.global HardFault_Handler
.global isr_hardfault
.type HardFault_Handler, %function
.thumb_func
HardFault_Handler:
isr_hardfault:
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
