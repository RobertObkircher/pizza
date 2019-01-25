#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int width;
    int height;
} Shape;

typedef struct {
    int r1, r2, c1, c2;
} Slice;

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

void run(const char *filename) {
    //
    // Read the file
    //
    char path[1024];
    sprintf(path, "inputs/%s.in", filename);
    FILE *input_file = fopen(path, "r");
    if (!input_file) {
        perror("input file");
        exit(1);
    }
    int R, C, L, H;
    fscanf(input_file, "%d %d %d %d\n", &R, &C, &L, &H);

    char pizza[R * C];
    for (int row = 0; row < R; ++row) {
        for (int col = 0; col < C; ++col) {
            pizza[row * C + col] = (char) getc(input_file);
        }
        getc(input_file);
    }
    fclose(input_file);

    //
    // Generate all possible shapes
    //
    int number_of_shapes = 0;
    Shape shapes[64];
    for (int x = H; x > 0; --x) {
        for (int y = H; y > 0; --y) {
            int area = x * y;
            if (area <= H && area >= 2 * L) {
                shapes[number_of_shapes++] = (Shape) {.width = x, .height=y};
            }
        }
    }

    //
    // Pick the first shape for each position and fill it with zeroes.
    //
    int num_slices = 0;
    int max_slices = 1024;
    Slice *slices = malloc(max_slices * sizeof(Slice));
    for (int pizza_row = 0; pizza_row < R; ++pizza_row) {
        for (int pizza_col = 0; pizza_col < C; ++pizza_col) {
            int pizza_index = pizza_row * C + pizza_col;
            char cell = pizza[pizza_index];

            if (!cell) // already used
                continue;

            for (int i = 0; i < number_of_shapes; ++i) {
                Shape shape = shapes[i];
                if (shape.width + pizza_col > C || shape.height + pizza_row > R)
                    continue;
                int mushrooms = 0;
                int tomatoes = 0;
                for (int y = 0; y < shape.height; ++y) {
                    for (int x = 0; x < shape.width; ++x) {
                        switch (pizza[pizza_index + y * C + x]) {
                            case 'M':
                                ++mushrooms;
                                break;
                            case 'T':
                                ++tomatoes;
                                break;
                            default:
                                goto next_shape;
                        }
                    }
                }
                next_shape:
                if (mushrooms + tomatoes == shape.width * shape.height && mushrooms >= L && tomatoes >= L) {
                    if (num_slices + 1 > max_slices) {
                        max_slices *= 2;
                        slices = realloc(slices, max_slices * sizeof(Slice));
                        if (!slices) {
                            printf("out of memory");
                            exit(1);
                        }
                    }
                    slices[num_slices++] = (Slice) {
                            .r1 = pizza_row,
                            .c1 = pizza_col,
                            .r2 = pizza_row + shape.height - 1,
                            .c2 = pizza_col + shape.width - 1
                    };
                    for (int y = 0; y < shape.height; ++y) {
                        for (int x = 0; x < shape.width; ++x) {
                            pizza[pizza_index + y * C + x] = 0;
                        }
                    }
                }
            }
        }
    }

    //
    // Write output file
    //
    {
        sprintf(path, "inputs/%s.out", filename);
        FILE *output_file = fopen(path, "w");
        if (!output_file) {
            perror("output file");
            exit(1);
        }
        fprintf(output_file, "%d\n", num_slices);
        for (int i = 0; i < num_slices; ++i) {
            Slice s = slices[i];
            fprintf(output_file, "%d %d %d %d\n", s.r1, s.c1, s.r2, s.c2);
        }
        fclose(output_file);
    }

    free(slices);
}