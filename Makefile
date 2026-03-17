all: dro

run: dro
	./dro

gdb: dro
	gdb --args ./dro

CXXFLAGS=-Iinclude -g -DDESKTOPSIM
LDFLAGS=-lSDL -g

OBJS = pcsrc/pcmain.o src/bmpfont.o src/gfx.o src/widgets.o src/screen.o src/calc.o src/eval.o src/buzzer.o src/touchscreen.o src/app.o src/encoder.o src/eeprom.o

src/bmpfont.o: include/bmpfont.h include/gfx.h
src/gfx.o: include/bmpfont.h include/gfx.h
src/buzzer.o: include/buzzer.h
src/touchscreen.o: include/touchscreen.h

src/widgets.o: include/bmpfont.h include/gfx.h include/widgets.h
src/screen.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h
src/eval.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eval.h
src/calc.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eval.h
src/eeprom.o: include/eeprom.h
src/app.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eeprom.h

pcsrc/pcmain.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h

src/bmpfont.cpp: src/fontconv.py
	(cd src; python3 fontconv.py)

clean:
	rm -f $(OBJS)

dro: $(OBJS)
	$(CXX) -o dro $^ $(LDFLAGS)

boot:
	(. ./activate; pio run -e tinydro_boot -t upload)
    
hw:
	(. ./activate; pio run -t upload)
    
debug:
	#(. ./activate; pio debug --interface=gdb -- -x .pioinit)
	(. ./activate; piodebuggdb -- -x .pioinit)

debug2:
	(. ./activate; pio debug -e tinydro2 --interface=gdb -- -x .pioinit)
