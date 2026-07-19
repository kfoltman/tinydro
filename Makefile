all: dro

run: dro
	./dro

gdb: dro
	gdb --args ./dro

CXXFLAGS=-Iinclude -g -DDESKTOPSIM
LDFLAGS=-lSDL -g

BSPOBJS = src/bsp/buzzer.o src/bsp/touchscreen.o src/bsp/encoder.o src/bsp/eeprom.o

LIBOBJS = src/lib/gfx.o src/lib/widgets.o src/lib/render.o src/lib/screen.o src/lib/calc.o src/lib/eval.o

DROOBJS = src/dro/app.o src/dro/droscreens.o

OBJS = pcsrc/pcmain.o ${DROOBJS} ${BSPOBJS} ${LIBOBJS} src/bmpfont.o

src/bsp/bmpfont.o: include/bmpfont.h include/gfx.h
src/bsp/gfx.o: include/bmpfont.h include/gfx.h
src/bsp/buzzer.o: include/buzzer.h
src/bsp/touchscreen.o: include/touchscreen.h
src/bsp/eeprom.o: include/eeprom.h

src/lib/render.o: include/bmpfont.h include/gfx.h include/widgets.h
src/lib/widgets.o: include/bmpfont.h include/gfx.h include/widgets.h
src/lib/screen.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h
src/lib/eval.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eval.h
src/lib/calc.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eval.h

src/dro/app.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eeprom.h include/droscreens.h
src/dro/droscreens.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/eeprom.h include/droscreens.h
pcsrc/pcmain.o: include/bmpfont.h include/gfx.h include/widgets.h include/screen.h include/droscreens.h

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

debug2b:
	(. ./activate; pio debug -e tinydro2_boot --interface=gdb -- -x .pioinit)
