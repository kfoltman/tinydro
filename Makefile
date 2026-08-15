all: dro

################################################################################################
# Native

run:
	(. ./activate; pio run -e tinydro_native -t exec)

gdb:
	(. ./activate; pio debug -e tinydro_native --interface gdb)

dro:
	(. ./activate; pio run -e tinydro_native)

clean:
	(. ./activate; pio run -e tinydro_native -t clean)

################################################################################################
# HW (various board versions, TBD)

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

debug4:
	(. ./activate; pio debug -e tinydro2_boot --interface=gdb -- -x .pioinit)
