.data
user_ok: .asciiz "admin\n"
pass_ok: .asciiz "1234\n"

user: .space 20
pass: .space 20

msg1: .asciiz "Usuario: "
msg2: .asciiz "Clave: "
msg3: .asciiz "Correcto\n"
msg4: .asciiz "Usuario o contraseña incorrecta\n"
msg5: .asciiz "Bloqueado, paz\n"

.text
.globl main

main:
    li $t0, 3   # intentos

inicio:
    beq $t0, $zero, bloqueado

    # pedir usuario
    li $v0, 4
    la $a0, msg1
    syscall

    li $v0, 8
    la $a0, user
    li $a1, 20
    syscall

    # pedir clave
    li $v0, 4
    la $a0, msg2
    syscall

    li $v0, 8
    la $a0, pass
    li $a1, 20
    syscall

    # comparar usuario
    la $a0, user
    la $a1, user_ok
    jal comparar
    bne $v0, $zero, mal

    # comparar clave
    la $a0, pass
    la $a1, pass_ok
    jal comparar
    bne $v0, $zero, mal

    # si todo bien
    li $v0, 4
    la $a0, msg3
    syscall
    j fin

mal:
    li $v0, 4
    la $a0, msg4
    syscall

    addi $t0, $t0, -1
    j inicio

bloqueado:
    li $v0, 4
    la $a0, msg5
    syscall

fin:
    li $v0, 10
    syscall

# comparar strings
comparar:
    lb $t1, 0($a0)
    lb $t2, 0($a1)

    bne $t1, $t2, diferente
    beq $t1, $zero, igual

    addi $a0, $a0, 1
    addi $a1, $a1, 1
    j comparar

diferente:
    li $v0, 1
    jr $ra

igual:
    move $v0, $zero
    jr $ra
