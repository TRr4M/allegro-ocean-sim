#/bin/bash

gcc ex_camera.c -o ex_camera $(pkg-config allegro-5 allegro_font-5 allegro_primitives-5 allegro_image-5 allegro_color-5 --libs --cflags) -lm
