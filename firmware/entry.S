/* entry.S - MIPS o32 PIC entry point */
.option pic2
.text
.globl __start
.type __start, @function
.ent __start

__start:
    .set    noreorder
    .cpload $t9             /* Setup $gp from $t9 */

    /* Align stack to 8 bytes */
    move    $fp, $sp
    addiu   $sp, $sp, -32
    and     $sp, $sp, -8
    .cprestore 16

    /* Pass argc ($a0) and argv ($a1) from kernel stack */
    lw      $a0, 0($fp)
    addiu   $a1, $fp, 4

    /* Call main */
    la      $t9, main
    jalr    $t9
    nop

    /* Call exit if main returns */
    move    $a0, $v0
    li      $v0, 4001       /* SYS_exit */
    syscall

    .set    reorder
.end __start
