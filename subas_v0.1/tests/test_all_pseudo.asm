SEGMENT CODE

    ORG 100h

    MOV AX, 100h
    ADD AX, 50h
    MOV BX, AX
    
    CMP BX, 100h
    JZ test_done
    DEC BX
    JMP test_start
    
test_start:
    MOV AX, 4C00h
    
test_done:
    INT 21h

    DB 01h
    DB 02h
    DB 03h
    
ENDS

END

