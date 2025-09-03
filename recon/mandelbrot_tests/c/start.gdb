file ./mandelbrot
#b debugeroni1
#b main
#break 367 if this_index == 37wat

#commands
#watch this_index if this_index > (RES_X * RES_Y)
#end
#b debugeroni2
#b 301
#b mandelbrot_bordertrace:BORDER_START
#commands
#commands
#watch current_bord_i if current_bord_i == (412 + (1 * 1600))
#end

#b 466
#commands
#  print FIXED_TO_DOUBLE(this_coord.r)
#  print FIXED_TO_DOUBLE(this_coord.i)
#  print this_index
#  continue
#end
#
#b 426
#commands
#  print FIXED_TO_DOUBLE(this_coord.r)
#  print FIXED_TO_DOUBLE(this_coord.i)
#  print this_index
#  continue
#end
#
#b 386
#commands
#  print this_index
#end

#watch this_index if this_index > (RES_X * RES_Y)

#break mandelbrot_bordertrace
#commands
#  watch pixels[8613]
#end

start_pwndbg
set gdb-workaround-stop-event 1
run

