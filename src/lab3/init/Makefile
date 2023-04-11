C_SRC       = $(sort $(wildcard *.c))
OBJ		    = $(patsubst %.c,%.o,$(C_SRC))

INCLUDE = -I $(shell pwd)/include -I $(shell pwd)/arch/riscv/include
file = main.o
all:$(OBJ)
	
%.o:%.c
	${GCC} ${CFLAG} -c -g $<
clean:
	$(shell rm *.o 2>/dev/null)
