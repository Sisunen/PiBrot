/*
The MIT License (MIT)

Copyright (c) 2014 Adam Simpson

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "fractal.h"
#include "communication.h"
#include "ogl_utils.h"
#include "multi_tex.h"
#include "stdio.h"
#include "string.h"
#include "renderer.h"
#include "exit_menu_gl.h"
#include "start_menu_gl.h"

int main(int argc, char *argv[])
{
    // Initialize MPI
    int myrank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    FRAC_INFO frac_left, frac_right;
    init_fractal(&frac_left, true, 1300);
    init_fractal(&frac_right, true, 1300);

    if (myrank == 0){

        // Setup initial OpenGL state
        gl_t gl_state;
        memset(&gl_state, 0, sizeof(gl_t));

        // Start OpenGL
        render_t render_state;
        init_ogl(&gl_state, &render_state);

        // Initialize render parameters
        render_state.started = false;
        render_state.quit_mode = false;
        render_state.screen_width = gl_state.screen_width;
        render_state.screen_height = gl_state.screen_height;
        render_state.return_value = 0;

        // Initialize exit menu
        exit_menu_t exit_menu_state;
        init_exit_menu(&exit_menu_state, &gl_state);
        render_state.exit_menu_state = &exit_menu_state;

        // Initialize start menu
        start_menu_t start_menu_state;
        init_start_menu(&start_menu_state, &gl_state);
        render_state.start_menu_state = &start_menu_state;

        // Setup texture state
        texture_t texture_state;
        memset(&texture_state, 0, sizeof(texture_t));
        texture_state.gl_state = &gl_state;

        // Create and set textures
        create_textures(&texture_state, &frac_left, &frac_right);
        // Create and set vertices
        create_vertices(&texture_state);
        // Create and set shaders
        create_shaders(&texture_state);

        // Initial Draw before data arrives
        draw_textures(&texture_state);

        // Swap front/back buffers
        swap_ogl(&gl_state);

        // Wait for user to unpause(start) simulation
        while(!render_state.started)
        {
            check_user_input(&gl_state);
            draw_textures(&texture_state);
            render_start_menu(&start_menu_state, render_state.mouse_x, render_state.mouse_y);
            swap_ogl(&gl_state);
        }

        // Start master loop
        master(&render_state, &frac_left, &frac_right, &texture_state);

        // Wait for key press
	while(!window_should_close(&gl_state)) {
            check_user_input(&gl_state);

            // Draw fractals
            draw_textures(&texture_state);

            // Render exit menu
            if(render_state.quit_mode)
                render_exit_menu(&exit_menu_state, render_state.mouse_x, render_state.mouse_y);

            // Swap buffers
            swap_ogl(&gl_state);
        }

        // Clean up and exit
        exit_ogl(&gl_state);
        exit_exit_menu(&exit_menu_state);

        MPI_Finalize();
        return render_state.return_value;

    }
    else if(myrank == 1)
	slave(&frac_left);
    else{
        slave(&frac_right);
    }

    MPI_Finalize();

    return 0;
}
