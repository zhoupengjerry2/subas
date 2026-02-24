; 完整功能测试 - 包含所有指令和伪指令

SEGMENT CODE
    ORG 100h

main:
    JMP start

; 测试数据
data_area:
    DB 01h
    DB 02h, 03h
    DB 0Fh

; 测试过程
helper:
    MOV AX, 1234h
    RET

start:
    ; 数据操作
    MOV AX, 100h
    ADD AX, 200h
    SUB AX, 50h
    
    ; 位操作
    MOV BX, 0FFh
    AND BX, 0Fh
    OR BX, 0F0h
    XOR BX, 0AAh
    
    ; 移位
    SHL AX, 1
    SHR BX, 2
    
    ; 比较和跳转
    CMP AX, 200h
    JZ equal_branch
    MOV CX, 0
    JMP not_equal_branch
equal_branch:
    MOV CX, 1
not_equal_branch:
    
    ; 其他跳转
    MOV DX, 100h
    CMP DX, 50h
    JNZ not_equal_test
    MOV AX, 0
not_equal_test:
    
    ; 进位跳转
    MOV AX, 0FFFFh
    ADD AX, 1
    JC carry_occurred
    JMP no_carry
carry_occurred:
    MOV BX, 1
    JMP done
no_carry:
    MOV BX, 0
    
    ; 无进位跳转
    MOV AX, 100h
    ADD AX, 1
    JNC no_carry_occurred
    MOV CX, 1
no_carry_occurred:
    
done:
    ; 标志操作
    CLC
    STC
    
    ; 循环
    MOV CX, 5
loop_start:
    ADD AX, 1
    LOOP loop_start
    
    ; 栈操作
    MOV AX, 1111h
    PUSH AX
    POP BX
    
    ; 中断
    MOV AX, 4C00h
    INT 21h

ENDS

END main
