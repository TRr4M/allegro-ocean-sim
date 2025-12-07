#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_opengl.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_image.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

#define pi ALLEGRO_PI

#define GRID_WIDTH  10
#define GRID_HEIGHT 10

typedef struct {
   float x, y, z;
} Vec3;

typedef struct {
   Vec3 position;
   Vec3 xaxis; /* This represent the direction looking to the right. */
   Vec3 yaxis; /* This is the up direction. */
   Vec3 zaxis; /* This is the direction towards the viewer ('backwards'). */
   double vertical_field_of_view; /* In radians. */
} Camera;


static double vector_dot_product(Vec3 a, Vec3 b) {
   return a.x * b.x + a.y * b.y + a.z * b.z;
}

static Vec3 vector_cross_product(Vec3 a, Vec3 b) {
   Vec3 v = {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
   return v;
}

static Vec3 vector_mul(Vec3 a, float s) {
   Vec3 v = {a.x * s, a.y * s, a.z * s};
   return v;
}

static double vector_norm(Vec3 a) {
   return sqrt(vector_dot_product(a, a));
}

static Vec3 vector_normalize(Vec3 a) {
   double s = vector_norm(a);
   if (s == 0)
      return a;
   return vector_mul(a, 1 / s);
}

/* In-place add another vector to a vector. */
static void vector_iadd(Vec3 *a, Vec3 b) {
   a->x += b.x;
   a->y += b.y;
   a->z += b.z;
}

Camera camera;
double mouse_look_speed = 0.03;
double movement_speed = 0.05;
int mouse_dx, mouse_dy;

ALLEGRO_VERTEX *vertices;
int *indices;

int width = 640;
int height = 480;
double frame_time = 1;

void handle_shader_errs(bool b, const char *msg, ALLEGRO_SHADER *shader) {
    if (!b) {
        fprintf(stderr, msg);
        fprintf(stderr, al_get_shader_log(shader));
        exit(1);
    }
}

void init_camera(Camera *camera) {
    camera->xaxis.x = 1;
    camera->yaxis.y = 1;
    camera->zaxis.z = 1;
    camera->vertical_field_of_view = 60 * pi / 180;
}

/* Rotate the camera around the given axis. */
static void camera_rotate_around_axis(Camera *c, Vec3 axis, double radians) {
   ALLEGRO_TRANSFORM t;
   al_identity_transform(&t);
   al_rotate_transform_3d(&t, axis.x, axis.y, axis.z, radians);
   al_transform_coordinates_3d(&t, &c->yaxis.x, &c->yaxis.y, &c->yaxis.z);
   al_transform_coordinates_3d(&t, &c->zaxis.x, &c->zaxis.y, &c->zaxis.z);

   /* Make sure the axes remain orthogonal to each other. */
   c->zaxis = vector_normalize(c->zaxis);
   c->xaxis = vector_cross_product(c->yaxis, c->zaxis);
   c->xaxis = vector_normalize(c->xaxis);
   c->yaxis = vector_cross_product(c->zaxis, c->xaxis);
}

void draw_ocean() {
    ALLEGRO_TRANSFORM t;
    ALLEGRO_TRANSFORM old_projection = *al_get_current_projection_transform();

    // Set up projection
    ALLEGRO_TRANSFORM projection;
    al_identity_transform(&projection);
    al_translate_transform_3d(&projection, 0, 0, -1);
    double f = tan(camera.vertical_field_of_view / 2);
    al_perspective_transform(&projection, -1 * width / height * f, f, 1, f * width / height, -f, 1000);
    al_use_projection_transform(&projection);

    // Use depth buffer
    al_set_render_state(ALLEGRO_DEPTH_TEST, 1);
    al_clear_depth_buffer(1);

    // Set up transform
    al_build_camera_transform(&t,
        camera.position.x, camera.position.y, camera.position.z,
        camera.position.x - camera.zaxis.x,
        camera.position.y - camera.zaxis.y,
        camera.position.z - camera.zaxis.z,
        camera.yaxis.x, camera.yaxis.y, camera.yaxis.z
    );
    al_use_transform(&t);

    // Draw
    // int n = al_draw_prim(vertices, NULL, NULL, 0, 3, ALLEGRO_PRIM_TRIANGLE_LIST);
    int num_vtx = (GRID_WIDTH - 1) * (GRID_HEIGHT - 1) * 6;
    int n = al_draw_indexed_prim(vertices, NULL, NULL, indices, num_vtx, ALLEGRO_PRIM_TRIANGLE_LIST);
    // printf("Drew %d tris\n", n);

    // Vec3 print_vector = camera.zaxis;
    // printf("<%f,%f,%f>\n", print_vector.x, print_vector.y, print_vector.z);

    // Restore projection
    al_identity_transform(&t);
    al_use_transform(&t);
    al_use_projection_transform(&old_projection);
    al_set_render_state(ALLEGRO_DEPTH_TEST, 0);
}

int main() {
    al_init();
    al_install_keyboard();
    al_init_primitives_addon();
    al_install_mouse();

    init_camera(&camera);

    // Manually created triangle, replace with some automatic stuff
    vertices = malloc(sizeof(ALLEGRO_VERTEX) * GRID_WIDTH * GRID_HEIGHT);
    for (int y = 0; y < GRID_HEIGHT; y++) {
        for (int x = 0; x < GRID_WIDTH; x++) {
            ALLEGRO_VERTEX vertex = vertices[y * GRID_WIDTH + x];
            vertex.x = x;
            vertex.y = -1;
            vertex.z = -y;
            vertex.color = al_map_rgb(0, 0, 255);
        }
    }
    // Try varying colors between vertices

    indices = malloc(sizeof(int) * (GRID_WIDTH - 1) * (GRID_HEIGHT - 1) * 6);
    for (int y = 0; y < GRID_HEIGHT - 1; y++) {
        for (int x = 0; x < GRID_WIDTH - 1; x++) {
            int ibase = y * (GRID_WIDTH - 1) * 6 + x * 6;
            int vbase = y * GRID_WIDTH + x;
            indices[ibase + 0] = vbase;
            indices[ibase + 1] = vbase + 1;
            indices[ibase + 2] = vbase + GRID_WIDTH;
            indices[ibase + 3] = vbase + 1;
            indices[ibase + 4] = vbase + GRID_WIDTH + 1;
            indices[ibase + 5] = vbase + GRID_WIDTH;
            // printf("%d, %d\n", ibase + 5, (GRID_WIDTH - 1) * (GRID_HEIGHT - 1) * 6);
        }
    }

    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST); // Try playing with this
    al_set_new_display_option(ALLEGRO_DEPTH_SIZE, 16, ALLEGRO_SUGGEST);
    al_set_new_display_flags(/*ALLEGRO_OPENGL |*/ ALLEGRO_RESIZABLE);

    ALLEGRO_TIMER* timer = al_create_timer(1.0 / 60.0);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();
    ALLEGRO_DISPLAY* disp = al_create_display(width, height);

    // ALLEGRO_SHADER *shader = al_create_shader(ALLEGRO_SHADER_GLSL);
    // handle_shader_errs(al_attach_shader_source_file(shader, ALLEGRO_PIXEL_SHADER, "fractal.frag"), "Unable to attach shader source file\n", shader);
    // handle_shader_errs(al_build_shader(shader), "Unable to build shader\n", shader);
    // handle_shader_errs(al_use_shader(shader), "Unable to use shader\n", shader);

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(disp));
    al_register_event_source(queue, al_get_timer_event_source(timer));
    al_register_event_source(queue, al_get_mouse_event_source());

    bool redraw = true;
    ALLEGRO_EVENT event;

    al_start_timer(timer);
    while(1) {
        al_wait_for_event(queue, &event);

        if (event.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;
            // camera_rotate_around_axis(&camera, camera.yaxis, 2 * pi / 60);
        }
        else if (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        } else if (event.type == ALLEGRO_EVENT_DISPLAY_SWITCH_IN) {
            redraw = true;
        } else if (event.type == ALLEGRO_EVENT_DISPLAY_RESIZE) {
            width = event.display.width;
            height = event.display.height;
            al_acknowledge_resize(disp);
            redraw = true;
        } else if (event.type == ALLEGRO_EVENT_MOUSE_AXES) {

        } else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {

        }

        if (redraw && al_is_event_queue_empty(queue)) {
            double start_time = al_get_time();
            al_clear_to_color(al_map_rgb(0, 0, 0));

            // Render
            draw_ocean();

            al_flip_display();
            double end_time = al_get_time();
            frame_time = end_time - start_time;

            redraw = false;
        }
    }

    // al_destroy_shader(shader);
    al_destroy_display(disp);
    al_destroy_timer(timer);
    al_destroy_event_queue(queue);

    return 0;
}
