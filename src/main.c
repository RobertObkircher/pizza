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

void *malloc_or_exit(size_t size) {
    void *result = malloc(size);
    if (!result) {
        printf("out of memory: unable to allocate %d bytes", size);
        exit(1);
    }
    return result;
}

void run(int);

int main() {
    clock_t start = clock();
    run(0);
    run(1);
    run(2);
    run(3);
    clock_t end = clock();
    double elapsed = (end - start) / (double) CLOCKS_PER_SEC;
    printf("Elapsed time: %.4f\n", elapsed);
    return 0;
}

const char *files[] = {
        "a_example",
        "b_small",
        "c_medium",
        "d_big",
};

FILE *open_file_or_exit(char *folder, int file_index, char *extension, char *mode) {
    char path[1024];
    sprintf(path, "%s/%s%s", folder, files[file_index], extension);
    FILE *file = fopen(path, mode);
    if (!file) {
        printf("Unable to open file %s\n", path);
        exit(1);
    }
    return file;
}

void run(int file_index) {
    FILE *file = open_file_or_exit("inputs", file_index, ".in", "r");
    int R, C, L, H;
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
    Shape *shapes;
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
        for (int x = 1; x <= H; ++x) {
            for (int y = 1; y <= H; ++y) {
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

    Data *possible_shapes = malloc_or_exit(R * C * sizeof(Data));
    {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                possible_shapes[pizza_index] = (Data) {};
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
                        possible_shapes[pizza_index].shape_flags |= 1 << i;
                    }
                }
            }
        }
    }

    {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                unsigned int bits = possible_shapes[pizza_index].shape_flags;
                for (int i = 0; bits; ++i, bits >>= 1) {
                    if (!(bits & 1))
                        continue;
                    Shape shape = shapes[i];
                    for (int y = 0; y < shape.height; ++y) {
                        for (int x = 0; x < shape.width; ++x) {
                            possible_shapes[pizza_index + y * C + x].num_shapes += 1;
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
                ++buckets[possible_shapes[pizza_index].num_shapes];
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
        FILE *fp = open_file_or_exit("inputs", file_index, "_num_shapes.png", "wb");
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
                Data d = possible_shapes[pizza_index];
                int n = d.num_shapes;
                if (n > max_num_shapes)
                    max_num_shapes = d.num_shapes;
            }
        }
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = possible_shapes[pizza_index];
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
        FILE *fp = open_file_or_exit("inputs", file_index, "_possible.png", "wb");
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
                Data d = possible_shapes[pizza_index];
                int c = bit_count(d.shape_flags);
                if (c > max_bit_count)
                    max_bit_count = c;
            }
        }
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data d = possible_shapes[pizza_index];
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

    int max_slices = 1024;
    int num_slices = 0;
    Slice *slices = malloc_or_exit(max_slices * sizeof(Slice));
    for (int current_num_shapes = 1; current_num_shapes <= max_num_shapes; ++current_num_shapes) {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                Data *s = &possible_shapes[pizza_index];
                if (s->is_used)
                    continue;
                unsigned int bits = s->shape_flags;
                if (!bits)
                    continue;
                Shape *best_shape = NULL;
                int best_shape_score = -1;
                for (int i = 0; i < number_of_shapes; ++i) {
                    if (!(bits & (1 << i)))
                        continue;

                    Shape *shape = &shapes[i];

                    int shape_can_be_placed = 1;
                    for (int y = 0; shape_can_be_placed && y < shape->height; ++y) {
                        for (int x = 0; shape_can_be_placed && x < shape->width; ++x) {
                            Data *data = &possible_shapes[pizza_index + y * C + x];
                            shape_can_be_placed = !data->is_used;
                            if (data->num_shapes > current_num_shapes) {
                                // a really weird place for this logic but it works.
                            }
                        }
                    }
                    if (shape_can_be_placed) {
                        best_shape = shape;
                    }
                }

                if (best_shape) {
                    if (num_slices + 1 > max_slices) {
                        max_slices *= 2;
                        slices = realloc(slices, max_slices * sizeof(Slice));
                        if (!slices) {
                            printf("out of memory");
                            exit(1);
                        }
                    }
                    Slice *slice = slices + num_slices++;
                    slice->r1 = pizza_row;
                    slice->c1 = pizza_col;
                    slice->r2 = pizza_row + best_shape->height - 1;
                    slice->c2 = pizza_col + best_shape->width - 1;

                    for (int y = 0; y < best_shape->height; ++y) {
                        for (int x = 0; x < best_shape->width; ++x) {
                            possible_shapes[pizza_index + y * C + x].is_used = 1;
                        }
                    }
                }
            }
        }
    }


    {
        FILE *output = open_file_or_exit("inputs", file_index, ".out", "w");
        fprintf(output, "%d\n", num_slices);
        for (int i = 0; i < num_slices; ++i) {
            Slice s = slices[i];
            fprintf(output, "%d %d %d %d\n", s.r1, s.c1, s.r2, s.c2);
        }
        fclose(output);
    }

    {
        FILE *fp = open_file_or_exit("inputs", file_index, "_leftovers.png", "wb");
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
                Data d = possible_shapes[pizza_index];
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


    free(possible_shapes);
    free(shapes);
    free(pizza);
}