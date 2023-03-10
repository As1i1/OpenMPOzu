#include <cstdio>
#include <fstream>
#include <cstring>
#include <string>
#include <omp.h>
#include <cmath>

using namespace std;

void print_error(string msg){
    fprintf(stderr, "Error: %s", msg.c_str());
    exit(1);
}

void calc(int &all_pixel_count, int* &pixel_count, int* &intensity_sum,
          int* &t_bounds, double &t_best_sigma, int f0){
    // поиск лучших границ
    for (int f1 = f0 + 1; f1 < 255; f1++) {
        for (int f2 = f1 + 1; f2 < 255; f2++) {
            double q1 = pixel_count[f0];
            double q2 = (pixel_count[f1] - pixel_count[f0]);
            double q3 = (pixel_count[f2] - pixel_count[f1]);
            double q4 = (pixel_count[255] - pixel_count[f2]);

            if (q1 == 0 || q2 == 0 || q3 == 0 || q4 == 0) {
                continue;
            }

            double u1 = intensity_sum[f0] / q1;
            double u2 = (intensity_sum[f1] - intensity_sum[f0]) / q2;
            double u3 = (intensity_sum[f2] - intensity_sum[f1]) / q3;
            double u4 = (intensity_sum[255] - intensity_sum[f2]) / q4;

            double u = intensity_sum[255] / all_pixel_count;

            double sigma = ((q1 * (u1 - u) * (u1 - u) +
                             q2 * (u2 - u) * (u2 - u)) +
                            (q3 * (u3 - u) * (u3 - u) +
                             q4 * (u4 - u) * (u4 - u))) / all_pixel_count;

            if (sigma > t_best_sigma) {
                t_bounds[0] = f0;
                t_bounds[1] = f1;
                t_bounds[2] = f2;
                t_best_sigma = sigma;
            }
        }
    }
}


void calc_without_omp(int row, int col, int hist[], int* &pixels,
                      int* &intensity_sum, int* &pixel_count, int *bounds, double &best_sigma){
    int all_pixel_count = row * col;

    for (int i = 0; i < row * col; i++) {
        hist[pixels[i]]++;
    }

    intensity_sum[0] = 0;
    for (int i = 1; i < 256; i++) {
        intensity_sum[i] = intensity_sum[i - 1] + i * hist[i];
    }
    pixel_count[0] = hist[0];
    for (int i = 1; i < 256; i++) {
        pixel_count[i] = pixel_count[i - 1] + hist[i];
    }

    for (int f0 = 0; f0 < 255; f0++) {
        calc(all_pixel_count, pixel_count, intensity_sum,
             bounds, best_sigma, f0);
    }

    for (int i = 0; i < all_pixel_count; ++i) {
        if (pixels[i] >= 0 && pixels[i] <= bounds[0]) {
            pixels[i] = 0;
        } else if (pixels[i] >= bounds[0] + 1 && pixels[i] <= bounds[1]) {
            pixels[i] = 84;
        } else if (pixels[i] >= bounds[1] + 1 && pixels[i] <= bounds[2]) {
            pixels[i] = 170;
        } else {
            pixels[i] = 255;
        }
    }
}


int main(int argc, char* argv[]) {
    int hist[256];
    memset(hist, 0, sizeof(hist));
    int *intensity_sum = new int[256];
    int *pixel_count = new int[256];
    int *pixels;
    int all_pixel_count;
    int row, col;
    int *bounds = new int[3];
    double best_sigma = 0.0;

//  открытие файла и проверка формата

    if (argc != 4) {
        print_error("Wrong number of arguments");
    }

    int thread_num = atoi(argv[1]) == 0 ? omp_get_max_threads() : atoi(argv[1]);

    ifstream in_file(argv[2], std::ios::binary);
    if (!in_file) {
        print_error("No such file");
    }
    string line_for_check;
    getline(in_file, line_for_check);
    if (line_for_check != "P5") {
        print_error("Unsupported file format");
    }
    in_file >> row >> col;
    getline(in_file, line_for_check);
    getline(in_file, line_for_check);
    if (line_for_check != "255") {
        print_error("Unsupported file format");
    }

    // чтение пикселей

    all_pixel_count = row * col;
    pixels = new int [row * col];

    for (int i = 0; i < row * col; i++) {
        char tmp[1];
        in_file.read(tmp, 1);
        pixels[i] = (uint8_t) tmp[0]; // char8_t
    }
    in_file.close();

    int t_hist[256];
    memset(t_hist, 0, sizeof(t_hist));

    int chunk_size = log2(row * col);
    int *t_bounds = new int[3];
    double t_best_sigma = 0.0;

    double start = omp_get_wtime();

    if (thread_num == -1) {
        calc_without_omp(row, col, hist, pixels, intensity_sum, pixel_count, bounds, best_sigma);
        printf("Time (%i thread(s)): %g ms\n", 1, (omp_get_wtime() - start) * 1000);
        goto WRITE;
    }


#pragma omp parallel if (atoi(argv[1]) != -1) firstprivate(t_hist)  num_threads(thread_num) default(shared)
    {
        // вычисление гистограммы
#pragma omp for schedule(dynamic, chunk_size) nowait
        for (int i = 0; i < row * col; i += 4) {
            t_hist[pixels[i]]++;
            t_hist[pixels[i + 1]]++;
            t_hist[pixels[i + 2]]++;
            t_hist[pixels[i + 3]]++;
        }
#pragma omp critical
        {
            for (int j = 0; j < 256; j += 4) {
                hist[j] += t_hist[j];
                hist[j + 1] += t_hist[j + 1];
                hist[j + 2] += t_hist[j + 2];
                hist[j + 3] += t_hist[j + 3];
            }
        }
#pragma omp barrier
#pragma omp single
        {
            // префиксные суммы для быстрого вычисления дисперсии
            intensity_sum[0] = 0;
            pixel_count[0] = hist[0];
            for (int i = 1; i < 256; i++) {
                intensity_sum[i] = intensity_sum[i - 1] + i * hist[i];
                pixel_count[i] = pixel_count[i - 1] + hist[i];
            }
        }

#pragma omp for schedule(dynamic, 6) nowait
        for (int f0 = 0; f0 < 255; f0++) {
            calc(all_pixel_count, pixel_count, intensity_sum,
                 t_bounds, t_best_sigma, f0);
        }
#pragma omp critical
        {
            if (t_best_sigma > best_sigma) {
                bounds[0] = t_bounds[0];
                bounds[1] = t_bounds[1];
                bounds[2] = t_bounds[2];
                best_sigma = t_best_sigma;
            }
        }
#pragma omp for schedule(dynamic, chunk_size)
        for (int i = 0; i < all_pixel_count; ++i) {
            if (pixels[i] >= 0 && pixels[i] <= bounds[0]) {
                pixels[i] = 0;
            } else if (pixels[i] >= bounds[0] + 1 && pixels[i] <= bounds[1]) {
                pixels[i] = 84;
            } else if (pixels[i] >= bounds[1] + 1 && pixels[i] <= bounds[2]) {
                pixels[i] = 170;
            } else {
                pixels[i] = 255;
            }
        }
    }

    printf("Time (%i thread(s)): %g ms\n", thread_num, (omp_get_wtime() - start) * 1000);

    WRITE:
// запись в файл
    ofstream out_file(argv[3], std::ios::binary);
    out_file << "P5\n";
    out_file << row << " " << col << "\n";
    out_file << "255\n";
    for (int i = 0; i < row * col; i++) {
        out_file.put((char) pixels[i]);
    }
    out_file.close();

    printf("%u %u %u\n", bounds[0], bounds[1], bounds[2]);

    delete[] intensity_sum;
    delete[] pixel_count;
    delete[] bounds;
    delete[] t_bounds;

    return 0;
}