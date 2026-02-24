SEGMENT CODE
    ORG 100h

main:
    JMP start

data_bytes:
    DB 00h
    DB 0FFh
    DB 010h
    DB 020h
    DB 030h

data_hex:
    DB 0Ah
    DB 0Bh
    DB 0Ch
    DB 0Fh

data_values:
    DB 1
    DB 2
    DB 3
    DB 10
    DB 100

label1:
    MOV AX, 0

label2:
    MOV BX, 0

start:
    MOV AX, data_bytes
    MOV BX, data_hex
    MOV CX, data_values
    
    CMP AX, label1
    JZ is_equal
    
    MOV DX, 0
    JMP end_test
    
is_equal:
    MOV DX, 1
    
end_test:
    MOV AX, 4C00h
    INT 21h

ENDS

END main
