; 伪指令和段语法测试（统一格式）
SEGMENT CODE
    ASSUME CS:CODE, DS:DATA
    ORG 100h

main PROC
    JMP test_seq

helper PROC
    MOV AX, 1234h
    RET
helper ENDP

test_seq:
    MOV AX, 100h
    ADD AX, 50h
    MOV BX, AX

    CMP BX, 100h
    JZ test_done
    SUB BX, 1
    JMP test_seq

test_done:
    MOV AX, 4C00h
    INT 21h

main ENDP
SEGMENT ENDS

SEGMENT DATA
    test_data DB 10h,20h,58h
SEGMENT ENDS

END main
