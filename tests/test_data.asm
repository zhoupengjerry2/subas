; 数据定义与寻址测试（统一段/伪指令）
SEGMENT CODE
    ASSUME CS:CODE, DS:DATA
    ORG 100h

main PROC
    JMP start

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

main ENDP
SEGMENT ENDS

SEGMENT DATA
    data_bytes DB 00h,0FFh,010h,020h,030h
    data_hex DB 0Ah,0Bh,0Ch,0Fh
    data_values DB 1,2,3,10,100
    label1 DB 0
    label2 DB 0
SEGMENT ENDS

END main
