source /usr/share/pwndbg/gdbinit.py
file build/stm32f1_buisnesscard_v1.elf
target extended localhost:3333 
load

#break mandelbrot.c:430
#commands
#  print cam
#end

