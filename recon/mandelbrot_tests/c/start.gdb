file ./mandelbrot
start_pwndbg
#b 301
b mandelbrot_bordertrace:BORDER_START
#b 327 if current_bord_i == 2012
#commands
#commands
#watch current_bord_i if current_bord_i == (412 + (1 * 1600))
#end
run
