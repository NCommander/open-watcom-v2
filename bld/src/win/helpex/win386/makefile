platform = Win16
CC = wcc386
CFLAGS = -zW
res1_flags = -bt=windows -30

!include ../helpex.mif
!include ../../master.mif

linkit : .PROCEDURE
    @%append $(LNK) system win386
    @%append $(LNK) option mindata=100K
    @%append $(LNK) option maxdata=100K
    @%append $(LNK) option stack=8k

bindit : .PROCEDURE
    wbind -q $(name) -R $(name).res

../helpex.hlp: ../helpex.rtf
    cd ..
    hc helpex
    cd win386
