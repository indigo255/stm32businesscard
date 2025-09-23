# delete file
file build/stm32f1_buisnesscard_v1.elf
target extended localhost:3333 

set logging file benchmark.txt
set logging enabled on

set $camtype = draw_mandelbrot()::cam
set $views = {$camtype, $camtype, $camtype, $camtype, $camtype}

# default view; lots of black
set $views[0].min_r = -1
set $views[0].min_i = -.5
set $views[0].max_r = 1
set $views[0].max_i = .5

# noise and complex curves
set $views[1].min_r = 3.1577176569116192
set $views[1].min_i = 0.22066305108751161
set $views[1].max_r = 3.1577882779434727
set $views[1].max_i = 0.22069836160343248

#noise at high iter acount
set $views[2].min_r = 3.1576023402469482
set $views[2].min_i = 0.22069267606110915
set $views[2].max_r = 3.1576029655564359
set $views[2].max_i = 0.22069298871585336

# sub mandelbrot asscrack
set $views[3].min_r = 1.5143340717923357
set $views[3].min_i = -1.4054856689592869e-05
set $views[3].max_r = 1.5143697529846534
set $views[3].max_i = 3.7857394692316253e-06

#set gdb-workaround-stop-event 1

break main:BENCHMARK_INIT
commands
  set $vi = 0
  while($vi < 4)
    print $views[$vi]
    set draw_mandelbrot()::cam = $views[$vi]
    set draw_mandelbrot()::view_mode = 0
    call draw_mandelbrot(0xff)
    print benchmark_stop()::time
    set $vi = $vi + 1
  end
end

load
continue

quit
