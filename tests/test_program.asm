; 综合功能测试程序（标准段/伪指令格式）
SEGMENT CODE
    ASSUME CS:CODE, DS:DATA
    ORG 100h

main PROC
    ; 算术
    MOV AX, 100h
    ADD AX, 50h
    SUB AX, 20h

    ; 位运算
    MOV BX, 0AFh
    AND BX, 0Fh
    OR BX, 05h
    XOR BX, 03h

    ; 移位
    SHL AX, 1
    SHR BX, 1

    ; 栈操作
    MOV CX, 5
    PUSH AX
    PUSH BX
    POP AX
    POP BX

    ; 条件跳转与CMP
    CMP AX, 100h
    JZ skip1
    MOV DX, 1
    JMP skip2
skip1:
    MOV DX, 0
skip2:

    MOV AX, 0FFh
    CMP AX, 0FEh
    JNZ skip3
    MOV AX, 1
skip3:

    ; 进位测试
    CLC
    MOV AX, 0FFFFh
    ADD AX, 1
    JC skip4
    JMP skip5
skip4:
    MOV BX, 1
    JMP skip6
skip5:
    MOV BX, 0
skip6:

    JNC cont
    MOV AX, 1
cont:

    MOV CX, 3
loop1:
    ADD AX, 1
    LOOP loop1

    STC
    CMP AX, 100h

    ; 结束并中断返回
    MOV AX, 4C00h
    INT 21h

    ; 数据（DB）示例
    DB 01h, 02h, 03h

main ENDP
SEGMENT ENDS

SEGMENT DATA
    ; 数据段占位（可选）
SEGMENT ENDS

END main
