#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

typedef struct {
    int width;
    int height;
} Shape;

typedef struct {
    int r1, r2, c1, c2;
} Slice;

#define SHAPE_FLAGS_BITS 20
#define NUM_SHAPES_BITS 11
typedef struct {
    unsigned int is_used : 1;
    unsigned int shape_flags : SHAPE_FLAGS_BITS;
    unsigned int num_shapes : NUM_SHAPES_BITS;
} Data;

int bit_count(unsigned int n) {
    int counter = 0;
    while (n) {
        ++counter;
        n &= (n - 1);
    }
    return counter;
}

int max(int a, int b) {
    return a > b ? a : b;
}

void *malloc_or_exit(size_t size) {
    void *result = malloc(size);
    if (!result) {
        printf("out of memory: unable to allocate %d bytes", size);
        exit(1);
    }
    return result;
}

void run(const char *);

int main() {
    clock_t start = clock();
    run("a_example");
    run("b_small");
    run("c_medium");
    run("d_big");
    clock_t end = clock();
    double elapsed = (end - start) / (double) CLOCKS_PER_SEC;
    printf("Elapsed time: %.4f\n", elapsed);
    return 0;
}

FILE *open_file_or_exit(char *folder, const char *filename, char *extension, char *mode) {
    char path[1024];
    sprintf(path, "%s/%s%s", folder, filename, extension);
    FILE *file = fopen(path, mode);
    if (!file) {
        printf("Unable to open file %s\n", path);
        exit(1);
    }
    return file;
}

static Data *shape_data;
static int R, C, L, H;
static Shape *shapes;
static int *solve_recursive_locations_x;
static int *solve_recursive_locations_y;
static int solve_recursive_locations_count;

//static double solve_recursive_shape_size;
//static double solve_recursive_leftovers;
static int solve_recursive_score;

static void solve_recursive(int index) {
    if (index == solve_recursive_locations_count) {
        // check if new top score
        int score = 0;
        unsigned int leftovers = 0;
        for (int i = 0; i < solve_recursive_locations_count; ++i) {
            Data *data = &shape_data[solve_recursive_locations_y[i] * C + solve_recursive_locations_x[i]];
            if (data->is_used)
                ++score;
            else
                ++leftovers;
        }

//        double leftovers_percent = leftovers / solve_recursive_shape_size;
//
//        if (leftovers_percent < solve_recursive_leftovers)
//            solve_recursive_leftovers = leftovers_percent;
        if (score > solve_recursive_score)
            solve_recursive_score = score;
    } else {
        int start_x = solve_recursive_locations_x[0];
        int start_y = solve_recursive_locations_y[0];

        Data *data = &shape_data[start_y * C + start_x];

        // not picking this location is always an option
        solve_recursive(index + 1);

        if (data->is_used)
            return;

        // for all possible shapes
        unsigned int flags = data->shape_flags;
        for (int i = 0; flags; ++i, flags >>= 1) {
            if (!(flags & 1))
                continue;

            Shape *shape = &shapes[i];
            int shape_end_x = start_x + shape->width;
            int shape_end_y = start_y + shape->height;

            // check if shape can be placed
            for (int y = start_y; y < shape_end_y; ++y) {
                for (int x = start_x; x < shape_end_x; ++x) {
                    if (shape_data[y * C + x].is_used)
                        goto next_shape;
                }
            }

            // place shape
            for (int y = start_y; y < shape_end_y; ++y) {
                for (int x = start_x; x < shape_end_x; ++x) {
                    shape_data[y * C + x].is_used = 1;
                }
            }

            // recursion
            solve_recursive(index + 1);

            // remove shape
            for (int y = start_y; y < shape_end_y; ++y) {
                for (int x = start_x; x < shape_end_x; ++x) {
                    shape_data[y * C + x].is_used = 0;
                }
            }
            next_shape:;
        }
    }
}


void run(const char *filename) {
    FILE *file = open_file_or_exit("inputs", filename, ".in", "r");
    fscanf(file, "%d %d %d %d\n", &R, &C, &L, &H);

    char *pizza = malloc_or_exit(R * C * sizeof(char));
    {
        for (int row = 0; row < R; ++row) {
            for (int col = 0; col < C; ++col) {
                pizza[row * C + col] = (char) getc(file);
            }
            getc(file);
        }

        printf("RCLH %d %d %d %d\n", R, C, L, H);
    }
    fclose(file);

    int number_of_shapes = 0;
    {
        for (int x = 1; x <= H; ++x) {
            for (int y = 1; y <= H; ++y) {
                int area = x * y;
                if (area <= H && area >= 2 * L) {
                    ++number_of_shapes;
                }
            }
        }
        shapes = malloc_or_exit(sizeof(Shape) * number_of_shapes);
        number_of_shapes = 0;
        for (int x = H; x > 0; --x) {
            for (int y = H; y > 0; --y) {
                int area = x * y;
                if (area <= H && area >= 2 * L) {
                    shapes[number_of_shapes++] = (Shape) {.width = x, .height=y};
                }
            }
        }
        printf("%d Shapes\n", number_of_shapes);
        if (SHAPE_FLAGS_BITS < number_of_shapes) {
            printf("not enough shape_flags_bits\n");
            exit(543);
        }
    }

    shape_data = malloc_or_exit(R * C * sizeof(Data));
    {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                shape_data[pizza_index] = (Data) {};
                for (int i = 0; i < number_of_shapes; ++i) {
                    Shape shape = shapes[i];
                    if (shape.width + pizza_col > C || shape.height + pizza_row > R)
                        continue;
                    int mushrooms = 0;
                    for (int y = 0; y < shape.height; ++y) {
                        for (int x = 0; x < shape.width; ++x) {
                            if (pizza[pizza_index + y * C + x] == 'M') {
                                ++mushrooms;
                            }
                        }
                    }
                    if (mushrooms >= L && shape.width * shape.height - mushrooms >= L) {
                        shape_data[pizza_index].shape_flags |= 1 << i;
                    }
                }
            }
        }
    }

    {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                unsigned int bits = shape_data[pizza_index].shape_flags;
                for (int i = 0; bits; ++i, bits >>= 1) {
                    if (!(bits & 1))
                        continue;
                    Shape shape = shapes[i];
                    for (int y = 0; y < shape.height; ++y) {
                        for (int x = 0; x < shape.width; ++x) {
                            shape_data[pizza_index + y * C + x].num_shapes += 1;
                        }
                    }

                }
            }
        }
    }

    {
        unsigned int buckets[1024] = {};
        unsigned int num_buckets = 1024;

        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                ++buckets[shape_data[pizza_index].num_shapes];
            }
        }

        while (num_buckets && !buckets[num_buckets - 1])
            --num_buckets;

        printf("%d Buckets: ", num_buckets);
        for (int i = 0; i < num_buckets; ++i) {
            printf("%d,", buckets[i]);
        }
        printf("\n");
    }

    int max_num_shapes = 0;
    {
        FILE *fp = open_file_or_exit("inputs", filename, "_num_shapes.png", "wb");
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, fp);
        int bit_depth = 8;
        int color_type = PNG_COLOR_TYPE_GRAY;
        png_set_IHDR(png_ptr, info_ptr, C, R,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        png_bytep row = malloc_or_exit(C);
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = shape_data[pizza_index];
                int n = d.num_shapes;
                if (n > max_num_shapes)
                    max_num_shapes = d.num_shapes;
            }
        }
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = shape_data[pizza_index];
                int n = d.num_shapes;
                if (n == 0) {
                    row[pizza_col] = 0;
                } else {
                    row[pizza_col] = (unsigned char) (127 + d.num_shapes * 127 / max_num_shapes);
                }
            }
            png_write_row(png_ptr, row);
        }

        png_write_end(png_ptr, NULL);
        fclose(fp);
    }

    {
        FILE *fp = open_file_or_exit("inputs", filename, "_possible.png", "wb");
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, fp);
        int bit_depth = 8;
        int color_type = PNG_COLOR_TYPE_GRAY;
        png_set_IHDR(png_ptr, info_ptr, C, R,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        png_bytep row = malloc_or_exit(C);
        int max_bit_count = 0;
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = shape_data[pizza_index];
                int c = bit_count(d.shape_flags);
                if (c > max_bit_count)
                    max_bit_count = c;
            }
        }
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = shape_data[pizza_index];
                int c = bit_count(d.shape_flags);
                if (c == 0) {
                    row[pizza_col] = 0;
                } else {
                    row[pizza_col] = (unsigned char) (127 + bit_count(d.shape_flags) * 127 / max_bit_count);
                }
            }
            png_write_row(png_ptr, row);
        }

        png_write_end(png_ptr, NULL);
        fclose(fp);
    }

    // output
    int max_slices = 1024;
    int num_slices = 0;
    Slice *slices = malloc_or_exit(max_slices * sizeof(Slice));

    // iterate diagonally from top to bottom
    for (int diagonal_start = 0; diagonal_start < R + C; ++diagonal_start) {
        int closest_to_top_left_x = diagonal_start;
        if (diagonal_start >= C)
            closest_to_top_left_x = C - 1;
        int closest_to_top_left_y = diagonal_start - C;
        if (diagonal_start < C)
            closest_to_top_left_y = 0;

        while (closest_to_top_left_x >= 0 && closest_to_top_left_y < R) {

            Slice slice = {};
//            double best_shape_leftovers = 2.0;
            int best_shape_score = -1;

            // select a few starting positions
            // 5 works well for big
            // 897497
            int size = 4;
            int look_before = 0;
            for (int start_y = max(0, closest_to_top_left_y - look_before);
                 start_y < closest_to_top_left_y + size && start_y < R; ++start_y) {
                for (int start_x = max(0, closest_to_top_left_x - look_before);
                     start_x < closest_to_top_left_x + size && start_x < C; ++start_x) {

                    Data *data = &shape_data[start_y * C + start_x];
                    if (data->is_used || !data->shape_flags)
                        continue;

                    // for all possible shapes
                    unsigned int flags = data->shape_flags;
                    for (int i = 0; flags; ++i, flags >>= 1) {
                        if (flags & 1) {
                            Shape *shape = &shapes[i];
                            int shape_end_x = start_x + shape->width;
                            int shape_end_y = start_y + shape->height;

                            // check if shape can be places
                            for (int y = start_y; y < shape_end_y; ++y) {
                                for (int x = start_x; x < shape_end_x; ++x) {
                                    if (shape_data[y * C + x].is_used)
                                        goto next_shape;
                                }
                            }

                            // place shape
                            for (int y = start_y; y < shape_end_y; ++y) {
                                for (int x = start_x; x < shape_end_x; ++x) {
                                    shape_data[y * C + x].is_used = 1;
                                }
                            }

                            // generate list of location below and to the right
                            // 2 cells border:
                            unsigned int num_locations = 0;
                            int locations_x[4 * H + 4];
                            int locations_y[4 * H + 4];
                            {
                                for (int i = 0; i < 2; ++i) {
                                    int below = shape_end_y + i;
                                    if (below < R) {
                                        for (int x = start_x; x < shape_end_x; ++x) {
                                            Data *d = &shape_data[below * C + x];
                                            if (!d->is_used && d->shape_flags) {
                                                locations_x[num_locations] = x;
                                                locations_y[num_locations] = below;
                                                ++num_locations;
                                            }
                                        }
                                    }
                                }
                                for (int i = 0; i < 2; ++i) {
                                    int right = shape_end_x + i;
                                    if (right < C) {
                                        for (int y = start_y; y < shape_end_y; ++y) {
                                            Data *d = &shape_data[y * C + right];
                                            if (!d->is_used && d->shape_flags) {
                                                locations_x[num_locations] = right;
                                                locations_y[num_locations] = y;
                                                ++num_locations;
                                            }
                                        }
                                    }
                                }
                                for (int i = 0; i < 2; ++i) {
                                    int below = shape_end_y + i;
                                    for (int j = 0; j < 2; ++j) {
                                        int right = shape_end_x + i;
                                        if (below < R && right < C) {
                                            locations_x[num_locations] = right;
                                            locations_y[num_locations] = below;
                                            ++num_locations;
                                        }
                                    }
                                }
                            }

                            /*
                            solve_recursive_leftovers = 2.0;
                            solve_recursive_shape_size = (shape->width + 2) * (shape->height + 2);
                             */
                            solve_recursive_score = 0;
                            solve_recursive_locations_x = locations_x;
                            solve_recursive_locations_y = locations_y;
                            solve_recursive_locations_count = num_locations;
                            solve_recursive(0);
//                            if (solve_recursive_leftovers < best_shape_leftovers) {
                            int area = shape->width * shape->height;
                            if (solve_recursive_score * area > best_shape_score) {
//                                best_shape_leftovers = solve_recursive_leftovers;
                                best_shape_score = solve_recursive_score * area;
                                slice.r1 = start_y;
                                slice.c1 = start_x;
                                slice.r2 = shape_end_y - 1;
                                slice.c2 = shape_end_x - 1;
                            }

                            // remove shape
                            for (int y = start_y; y < shape_end_y; ++y) {
                                for (int x = start_x; x < shape_end_x; ++x) {
                                    shape_data[y * C + x].is_used = 0;
                                }
                            }
                        }
                        next_shape:;
                    }

                }
            }

            // add slices to output
//            if (best_shape_leftovers <= 1) {
            if (best_shape_score >= 0) {
                if (num_slices + 1 > max_slices) {
                    max_slices *= 2;
                    slices = realloc(slices, max_slices * sizeof(Slice));
                    if (!slices) {
                        printf("out of memory");
                        exit(1);
                    }
                }

                slices[num_slices++] = slice;

                for (int y = slice.r1; y <= slice.r2; ++y) {
                    for (int x = slice.c1; x <= slice.c2; ++x) {
                        shape_data[y * C + x].is_used = 1;
                    }
                }
            }

            // compute next location
            --closest_to_top_left_x;
            ++closest_to_top_left_y;
        }
    }

    {
        FILE *output = open_file_or_exit("inputs", filename, ".out", "w");
        fprintf(output, "%d\n", num_slices);
        for (int i = 0; i < num_slices; ++i) {
            Slice s = slices[i];
            fprintf(output, "%d %d %d %d\n", s.r1, s.c1, s.r2, s.c2);
        }
        fclose(output);
    }

    {
        FILE *fp = open_file_or_exit("inputs", filename, "_leftovers.png", "wb");
        png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        png_infop info_ptr = png_create_info_struct(png_ptr);
        png_init_io(png_ptr, fp);
        int bit_depth = 8;
        int color_type = PNG_COLOR_TYPE_GRAY;
        png_set_IHDR(png_ptr, info_ptr, C, R,
                     bit_depth, color_type, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
        png_write_info(png_ptr, info_ptr);

        int score = 0;
        png_bytep row = malloc_or_exit(C);
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = shape_data[pizza_index];
                if (d.is_used) {
                    row[pizza_col] = 0;
                    ++score;
                } else {
                    row[pizza_col] = 255;
                }
            }
            png_write_row(png_ptr, row);
        }

        png_write_end(png_ptr, NULL);
        fclose(fp);

        printf("Score: %d\n", score);
    }


    free(shape_data);
    free(shapes);
    free(pizza);
}