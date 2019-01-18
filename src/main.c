#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int width;
    int height;
} Shape;

typedef struct {
    int r1, r2, c1, c2;
} Slice;

int bit_count(unsigned long long n) {
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
    run(0);
    run(1);
    run(2);
    run(3);

    return 0;
}

void run(int file_index) {
    const char *files[] = {
            "inputs/a_example.in",
            "inputs/b_small.in",
            "inputs/c_medium.in",
            "inputs/d_big.in",
    };
    const char *output_files[] = {
            "inputs/a_example.out",
            "inputs/b_small.out",
            "inputs/c_medium.out",
            "inputs/d_big.out",
    };

    printf("file: %s\n", files[file_index]);
    FILE *file = fopen(files[file_index], "r");
    if (!file) {
        printf("Unable to open file!");
        exit(1);
    }

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
//        for (int row = 0; row < R; ++row) {
//            for (int col = 0; col < C; ++col) {
//                printf("%c", *(pizza + row * C + col));
//            }
//            printf("\n");
//        }
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
//        for (int i = 0; i < number_of_shapes; ++i) {
//            printf("%2d: %d %d\n", i, shapes[i].width, shapes[i].height);
//        }
    }

    long long *possible_shapes = malloc_or_exit(R * C * sizeof(long long));
    {
        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
                int pizza_index = pizza_row * C + pizza_col;
                possible_shapes[pizza_index] = 0;
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
                        possible_shapes[pizza_index] |= 1 << i;
                    }
                }
            }
        }

//        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
//            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
//                printf("%8x ", possible_shapes[pizza_row * C + pizza_col]);
//            }
//            printf("\n");
//        }

//        for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
//            for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
//                printf("Row %2d, Col %2d: ", pizza_row, pizza_col);
//                unsigned long long shape = possible_shapes[pizza_row * C + pizza_col];
//                for (int i = 0; i < number_of_shapes; ++i) {
//                    if (shape & (1 << i)) {
//                        printf("(%d %d)", shapes[i].width, shapes[i].height);
//                    }
//                }
//                printf("\n");
//            }
//            printf("\n");
//        }
    }

    /*
     for all shapes
       compute number of shapes to the right
       compute number of shapes below
       the border counts as num_shapes possible shapes
     pick shape with largest sum of neighbours
     */
    int max_slices = 1024;
    int num_slices = 0;
    Slice *slices = malloc_or_exit(max_slices * sizeof(Slice));
    for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
        for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
            int pizza_index = pizza_row * C + pizza_col;
            long long bits = possible_shapes[pizza_index];
            if (!bits)
                continue;
            Shape *best_shape = NULL;
            int best_shape_score = -1;
            for (int i = 0; i < number_of_shapes; ++i) {
                if (!(bits & (1 << i)))
                    continue;

                Shape *shape = &shapes[i];
                int score = 0;

                if (pizza_row == 0) {
                    score += shape->width;
                }
                if (pizza_col == 0) {
                    score += shape->height;
                }

                if (shape->width + pizza_col == C) {
                    score += shape->height;
                } else {
                    unsigned long long neighbours = 0;
                    for (int y = 0; y < shape->height && pizza_row + y < R; ++y) {
                        neighbours |= possible_shapes[pizza_index + y * C + shape->width];
                    }
                    score += bit_count(neighbours);
                }

                if (shape->height + pizza_row == R) {
                    score += shape->width;
                } else {
                    unsigned long long neighbours = 0;
                    for (int x = 0; x < shape->width && pizza_col + x < C; ++x) {
                        neighbours |= possible_shapes[pizza_index + x + shape->height * C];
                    }
                    score += bit_count(neighbours);
                }

                if (score > best_shape_score) {
                    best_shape_score = score;
                    best_shape = shape;
                }
            }

            if (best_shape) {
                if (num_slices + 1 > max_slices) {
                    max_slices *= 2;
//                    printf("realloc: %d\n", max_slices * sizeof(Slice));
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

//                printf("crwh: %d %d %d %d\n", pizza_col, pizza_row, best_shape->width, best_shape->height);
                for (int y = 0; y < best_shape->height; ++y) {
                    for (int x = 0; x < best_shape->width; ++x) {
                        possible_shapes[pizza_index + y * C + x] = 0;
                    }
                }

//                printf("the best shape at pos (%d, %d) was (%d %d)\n", pizza_row, pizza_col, best_shape->width, best_shape->height);
//                for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
//                    for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
//                        printf("Row %2d, Col %2d: ", pizza_row, pizza_col);
//                        unsigned long long shape = possible_shapes[pizza_row * C + pizza_col];
//                        for (int i = 0; i < number_of_shapes; ++i) {
//                            if (shape & (1 << i)) {
//                                printf("(%d %d)", shapes[i].width, shapes[i].height);
//                            }
//                        }
//                        printf("\n");
//                    }
//                    printf("\n");
//                }
            }
        }
    }


    FILE *output = fopen(output_files[file_index], "w");
    if (!output) {
        perror("Writing output");
        exit(2);
    }
    fprintf(output, "%d\n", num_slices);
    for (int i = 0; i < num_slices; ++i) {
        Slice s = slices[i];
        fprintf(output, "%d %d %d %d\n", s.r1, s.c1, s.r2, s.c2);
    }
    fclose(output);


    free(possible_shapes);
    free(shapes);
    free(pizza);
}