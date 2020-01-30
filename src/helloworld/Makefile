CC=bcm2708hardfp-gcc
CARD=23
NAME=agon_lelouet
CFLAGS=-W -Wall -Wextra -Wfatal-errors -O2 -static
APP=helloworld lab1/blink0

all: $(APP)
	scp -P 622$(CARD) $^ pi@peri:$(NAME)

$(APP): $(APP).c
	$(CC) -o $@ $< $(CFLAGS)
# on fait une compilation explicite par souci de clarté, mais GNU make sait faire de la
# "compilation implicite"

clean:
	rm $(APP)