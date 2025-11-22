PROG=proj

.PATH: $(.CURDIR)/timer
.PATH: $(.CURDIR)/keyboard
.PATH: $(.CURDIR)/video
.PATH: $(.CURDIR)/menu
.PATH: $(.CURDIR)/game
.PATH: $(.CURDIR)/highscore


SRCS = main.c timer.c keyboard.c graphics.c tile.c utils.c KBC.c menu.c options.c game.c highscore.c

CFLAGS += -pedantic 

# list of library dependencies 
DPADD += ${LIBLCF}
LDADD += -Wall -llcf -lxpm -llm
.include <minix.lcom.mk>

