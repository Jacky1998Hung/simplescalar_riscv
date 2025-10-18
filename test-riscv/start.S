# start.S — raw syscalls: write(1,"hi\n",3); exit(0)
    .option nopic
    .text
    .globl _start
_start:
    # a1 = &msg
    la   a1, msg
    # a2 = (&msg_end) - (&msg)
    la   t0, msg_end
    la   t1, msg
    sub  a2, t0, t1

    # write(1, msg, len)
    li   a0, 1          # fd = 1
    li   a7, 64         # __NR_write
    ecall

    # exit(0)
    li   a0, 0
    li   a7, 93         # __NR_exit
    ecall

    .section .rodata
    .align 4
msg:
    .ascii "hi\n"
msg_end:

