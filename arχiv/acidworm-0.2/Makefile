
PREFIX=/usr/games
BINDIR=$(PREFIX)
PROGRAM=acidworm

# -fno-strength-reduce is there for a reason - bug in GCC <2.7.2 and gives us one more register on i386 machines 
CC=gcc
CFLAGS= -pipe -fno-strength-reduce -O6 -fcaller-saves -fforce-mem -fforce-addr -funroll-loops -fomit-frame-pointer -s 
LIBS=	-lvgagl -lvga -lm

# -------- No need to edit anything below here ---------------------------
start: message
message: 
	@echo ""
	@echo "AcidWorm compile options"
	@echo "By Aaron Tiensivu for SpunkMunky International (12/05/96)"
	@echo ""
	@echo "   make acidworm       compile acidworm"
	@echo "   make install        install acidworm"
	@echo "   make clean          clean up files"
	@echo ""

all: $(PROGRAM)

$(PROGRAM):
	$(CC) $(CFLAGS) -o $(PROGRAM) $(PROGRAM).c $(LIBS)

install:
	install -c -s -m 0755 acidworm $(BINDIR)
clean:
	rm -f *.o
	rm -f core 
	rm -f $(PROGRAM)
