.data
mensaje1: .asciiz "Adivina el numero (1-10): "
mayor:    .asciiz "Muy alto\n"
menor:    .asciiz "Muy bajo\n"
ganaste:  .asciiz "¡Ganaste!\n"

numeroSecreto: .word 7

.text
.globl main

main:

loop:

    # Mostrar mensaje
    li $v0, 4
    la $a0, mensaje1
    syscall

    # Leer numero
    li $v0, 5
    syscall
    move $t0, $v0

    # Cargar numero secreto
    lw $t1, numeroSecreto

    # Comparar
    beq $t0, $t1, correcto

    blt $t0, $t1, muyBajo

muyAlto:
    li $v0, 4
    la $a0, mayor
    syscall
    j loop

muyBajo:
    li $v0, 4
    la $a0, menor
    syscall
    j loop

correcto:
    li $v0, 4
    la $a0, ganaste
    syscall

    # Salir
    li $v0, 10
    syscall