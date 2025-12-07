#/bin/bash

gcc main.c -o main $(pkg-config allegro-5 allegro_font-5 allegro_primitives-5 allegro_image-5 --libs --cflags) -lm
