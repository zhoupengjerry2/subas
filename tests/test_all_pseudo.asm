; 完整伪指令综合测试（标准化）
SEGMENT CODE
    ASSUME CS:CODE, DS:DATA
    ORG 100h

main PROC
    MOV AX, 100h
    ADD AX, 50h
    MOV BX, AX

test_loop:
    CMP BX, 100h
    JZ test_done
    SUB BX, 1
    JMP test_loop

test_done:
    MOV AX, 4C00h
    INT 21h

main ENDP
SEGMENT ENDS

SEGMENT DATA
    test_data DB 01h,02h,03h
SEGMENT ENDS

END main

