NAME = agon_lelouet
RPI ?= 23

CROSS_COMPILE ?= bcm2708hardfp-

CFLAGS=-Wall -Wfatal-errors -O2 -pthread
LDFLAGS=-static

BIN=bin/blink0.x bin/blink0_pt.x bin/blink01_pt.x bin/read_bp.x bin/blink01_bp_pt.x

help:
	@echo ""
	@echo "make [RPI=num] target"
	@echo ""
	@echo "      target :	help	this message"
	@echo " 		all	clean all and recompile"
	@echo " 		dep	compile if required"
	@echo " 		clean	clean all except sources"
	@echo " 		copy	compile then copy to card $(RPI) in directory $(NAME)"
	@echo ""

all: clean dep 

dep : $(BIN)

copy: $(BIN)
	scp -P 622$(RPI) $^ pi@peri:$(NAME)

bin/%.x: %.c
	$(CROSS_COMPILE)gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

clean:
	rm -f *.o *.x *~ *.a
