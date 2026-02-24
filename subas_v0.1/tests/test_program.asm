; 综合功能测试程序
; 包含所有主要指令和伪指令

_TEXT   SEGMENT
    ASSUME CS:_TEXT, DS:_TEXT
    ORG 100h
main   PROC
    MOV AX, 100h
    ADD AX, 50h
    SUB AX, 20h
    
    MOV BX, 0AFh
    AND BX, 0Fh
    OR BX, 05h
    XOR BX, 03h
    
    SHL AX, 1
    SHR BX, 1
    
    MOV CX, 5
    PUSH AX
    PUSH BX
    POP AX
    POP BX
    
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
    
    MOV AX, 4C00h
    INT 21h
    
    DB 01h, 02h, 03h

main    ENDP

_TEXT   ENDS

END start
