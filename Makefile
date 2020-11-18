FILES = assembler.c main.c util.c 
INCLUDES = -I.
CFLAGS = -std=c99
OUTFILE = assembler

main: ${FILES}
	${CC} ${CFLAGS} ${FILES} -o ${OUTFILE}
