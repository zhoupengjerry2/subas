SEGMENT CODE
    ORG 100h

main:
    JMP test_seq

helper:
    MOV AX, 1234h
    RET

test_data:
    DB 10h
    DB 20h
    DB 58h

test_seq:
    MOV AX, 100h
    ADD AX, 50h
    MOV BX, AX
    
    CMP BX, 100h
    JZ test_done
    DEC BX
    JMP test_seq
    
test_done:
    MOV AX, 4C00h
    INT 21h
    
    DB 0
    
ENDS

END main
