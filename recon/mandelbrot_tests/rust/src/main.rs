//let varname:[type] = value
//fn (name:type) -> ret type {
extern crate raylib;
use raylib::prelude::*;

struct Camera {
    min_i:f64,
    max_i:f64,
    min_r:f64,
    max_r:f64,
}

#[inline]
fn get_color(i:i32) -> Color {
    println!("a");
    return Color::BLUE;
    if i == ITERS || i == 0 { return Color::BLACK; }
    if i < 128 {
        return Color{r:(2*(i - 128)+255) as u8, 
            g:0, 
            b:(4*(i-64)+255) as u8, 
            a:255 };
    }
    Color{r:0, g:0, b:(-2*(i-128)+255) as u8, a:255}
}

fn shift_cam(cam: &mut Camera, step_r:f64, step_i:f64) {
    let i_offset = (cam.max_i - cam.min_i) * step_i;
    let r_offset = (cam.max_r - cam.min_r) * step_r;
    cam.min_i += i_offset;
    cam.max_i += i_offset;
    cam.min_r += r_offset;
    cam.max_r += r_offset;
}

fn zoom_cam(cam: &mut Camera, zoom: f64) {
    let i_scale = (cam.max_i - cam.min_i) * zoom;
    let r_scale = (cam.max_r - cam.min_r) * zoom;
    cam.min_i += i_scale;
    cam.max_i += i_scale;
    cam.min_i += r_scale;
    cam.max_i += r_scale;
}

const DECIMAL_LOC:i32 = 28;
const F64_SCALER:i32 = 1 << DECIMAL_LOC;

#[inline]
fn double_to_fixed(val:f64) -> i32 {
    (val * (1 << DECIMAL_LOC) as f64) as i32
}

#[inline]
fn fixed_multiply(x:i32, y:i32) -> i32 {
    ((x as i64 * y as i64) >> DECIMAL_LOC) as i32
}
const ITERS:i32 = 255;

//I'll learn macros later
const CUTOFF_SQUARED:i32 = (4f64 * (1 << DECIMAL_LOC) as f64) as i32;

fn main() {
    const RENDER_SIZE_X:i32 = 160;
    const RENDER_SIZE_Y:i32 = 80;
    const WINDOW_SIZE_X:i32 = RENDER_SIZE_X * 10;
    const WINDOW_SIZE_Y:i32 = RENDER_SIZE_Y * 10;
    const STEP_SIZE:f64 = 0.1;
    const ZOOM_SIZE:f64 = 0.1;

    let mut cam = Camera {min_i:-1.0, max_i:1.0, min_r:0.0, max_r:0.0};
    let mut pixels:[Color; (RENDER_SIZE_X * RENDER_SIZE_Y) as usize] = unsafe {std::mem::MaybeUninit::uninit().assume_init() };
    let (mut rl, thread) = raylib::init()
        .size(WINDOW_SIZE_X, WINDOW_SIZE_Y)
        .title("Mandelbrot stuff")
        .build();
    rl.set_target_fps(10);
    let mut tex:Texture2D;
    {
        let img:Image = texture::Image::gen_image_color(RENDER_SIZE_X, RENDER_SIZE_Y, Color::WHITE);
        tex = rl.load_texture_from_image(&thread, &img).unwrap();
    }
    while !rl.window_should_close() {
        let mut display = rl.begin_drawing(&thread);
        match display.get_key_pressed() {
            Some(KeyboardKey::KEY_UP) => shift_cam(&mut cam, 0.0, STEP_SIZE),
            Some(KeyboardKey::KEY_DOWN) => shift_cam(&mut cam, 0.0, -STEP_SIZE),
            Some(KeyboardKey::KEY_RIGHT) => shift_cam(&mut cam, STEP_SIZE, 0.0),
            Some(KeyboardKey::KEY_LEFT) => shift_cam(&mut cam, -STEP_SIZE, 0.0),
            Some(KeyboardKey::KEY_W) => zoom_cam(&mut cam, ZOOM_SIZE),
            Some(KeyboardKey::KEY_S) => zoom_cam(&mut cam, -ZOOM_SIZE),
            _ => continue,
        }
        println!("frame");
        {
            let scale_i:i32 = double_to_fixed((cam.max_i - cam.min_i) as f64 / RENDER_SIZE_Y as f64);
            let scale_r:i32 = double_to_fixed((cam.max_r - cam.min_r) as f64 / RENDER_SIZE_X as f64);
            let mut c_i:i32 = double_to_fixed(cam.max_i);
            let (mut c_r, mut z_i, mut z_r, mut zn_i, mut zn_r):(i32, i32, i32, i32, i32);
            let (mut z_r_2, mut z_i_2):(i32, i32);
            let mut i:i32 = 0;
            for y in 0..RENDER_SIZE_Y {
                c_r = double_to_fixed(cam.min_r);
                for x in 0..RENDER_SIZE_X {
                    z_i = 0;
                    z_r = 0;
                    for i in 0..ITERS {
                        z_r_2 = fixed_multiply(z_r, z_r);
                        z_i_2 = fixed_multiply(z_i, z_i);
                        zn_r = z_r_2 - z_i_2 + c_r;
                        zn_i = fixed_multiply(double_to_fixed(2f64), fixed_multiply(z_r, z_i)) + c_i;
                        z_i = zn_i;
                        z_r = zn_r;
                        if z_i_2 + z_r_2 > CUTOFF_SQUARED { break; };
                    }
                    pixels[((y * RENDER_SIZE_Y) + x) as usize] = Color::BLUE;
                    c_r += scale_r;
                }
                c_i -= scale_i;
            }
        }
        let byte_slice: &[u8] = unsafe {
            std::slice::from_raw_parts(pixels.as_ptr() as *const u8, pixels.len())
        };
        tex.update_texture(byte_slice);
        println!("{}", (display.get_render_width() as f32/RENDER_SIZE_X as f32) as f32);
        display.draw_texture_ex(&tex, Vector2{x:0.0, y:0.0}, 0.0, (display.get_render_width() as f32/RENDER_SIZE_X as f32) as f32, Color::WHITE);
        println!("done");
    }
}

