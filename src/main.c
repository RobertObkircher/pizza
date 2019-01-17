#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int width;
    int height;
} Shape;

int bit_count(unsigned long long n) {
    int counter = 0;
    while (n) {
        ++counter;
        n &= (n - 1);
    }
    return counter;
}

int main() {

    int file_index = 0;
    const char *files[] = {
            "inputs/a_example.in",
            "inputs/b_small.in",
            "inputs/c_medium.in",
            "inputs/d_big.in",
    };

    printf("file: %s\n", files[file_index]);
    FILE *file = fopen(files[file_index], "r");
    if (!file) {
        printf("Unable to open file!");
        exit(1);
    }

    int R, C, L, H;
    fscanf(file, "%d %d %d %d\n", &R, &C, &L, &H);

    char *pizza = malloc(R * C * sizeof(char));
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

    int number_of_shapes = 0;
    Shape *shapes;
    {
        for (int x = 1; x <= H; ++x) {
            for (int y = 1; y <= H; ++y) {
                if (x * y <= H) {
                    ++number_of_shapes;
                }
            }
        }
        shapes = malloc(sizeof(Shape) * number_of_shapes);
        number_of_shapes = 0;
        for (int x = 1; x <= H; ++x) {
            for (int y = 1; y <= H; ++y) {
                if (x * y <= H) {
                    shapes[number_of_shapes++] = (Shape) {.width = x, .height=y};
                }
            }
        }
        printf("%d Shapes\n", number_of_shapes);
//        for (int i = 0; i < number_of_shapes; ++i) {
//            printf("%d %d\n", shapes[i].width, shapes[i].height);
//        }
    }

    long long *possible_shapes = malloc(R * C * sizeof(long long));
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
    }

    /*
     for all shapes
       compute number of shapes to the right
       compute number of shapes below
       the border counts as num_shapes possible shapes
     pick shape with largest sum of neighbours
     */
    for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
        for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
            int pizza_index = pizza_row * C + pizza_col;
            if (!possible_shapes[pizza_index])
                continue;
            Shape *best_shape = NULL;
            int best_shape_score = -1;
            for (int i = 0; i < number_of_shapes; ++i) {
                if (!possible_shapes[pizza_index] & (1 << i))
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
                    for (int y = 0; y < shape->height; ++y) {
                        neighbours |= possible_shapes[pizza_index + y * C + shape->width];
                    }
                    score += bit_count(neighbours);
                }

                if (shape->height + pizza_row == C) {
                    score += shape->width;
                } else {
                    unsigned long long neighbours = 0;
                    for (int x = 0; x < shape->width; ++x) {
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
                // TODO store shape in list
                printf("crwh: %d %d %d %d\n", pizza_col, pizza_row, best_shape->width, best_shape->height);
                for (int y = 0; y < best_shape->height; ++y) {
                    for (int x = 0; x < best_shape->width; ++x) {
                        possible_shapes[pizza_index + y * C + x] = 0;
                    }
                }
            }
        }
    }


    free(shapes);
    free(pizza);
    return 0;
}