# Programa MIPS - 8 instrucciones
# Para ejecutar en MARS MIPS Simulator

.data
    memoria: .word 0       # espacio en RAM para sw/lw

.text
.globl main
main:
    addi $t0, $zero, 5     # $t0 = 5
    addi $t1, $zero, 3     # $t1 = 3
    add  $t2, $t0, $t1     # $t2 = 5 + 3 = 8
    sub  $t3, $t0, $t1     # $t3 = 5 - 3 = 2
    sw   $t2, memoria      # RAM[memoria] = 8
    lw   $t4, memoria      # $t4 = RAM[memoria] = 8
    and  $t5, $t0, $t1     # $t5 = 5 AND 3 = 1
    or   $t6, $t0, $t1     # $t6 = 5 OR  3 = 7

    # Fin del programa
    li   $v0, 10
    syscall
