; 高级指令测试（统一段/伪指令）
SEGMENT CODE
    ASSUME CS:CODE, DS:DATA
    ORG 100h

main PROC
    ; 基础寄存器初始化
    MOV AX, 12h
    MOV BX, 8h

    MOV AX, 100h
    MOV BX, 10h

    MOV AX, 100h
    MOV BX, 200h
    CMP AX, BX
    JNZ not_zero
    MOV AX, 0
    JMP end1
not_zero:
    MOV AX, 1
end1:

    MOV AX, 0FFFFh
    ADD AX, 1
    JC carry_flag
    MOV BX, 0
    JMP end2
carry_flag:
    MOV BX, 1
end2:

    MOV AX, 0
    ADD AX, 1
    JNC no_carry
    MOV CX, 0
    JMP end3
no_carry:
    MOV CX, 1
end3:

    MOV AX, 4C00h
    INT 21h

main ENDP
SEGMENT ENDS

SEGMENT DATA
    ; 数据段占位
SEGMENT ENDS

END main
