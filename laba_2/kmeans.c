#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

typedef struct {
    double x, y;
} Point;

typedef struct {
    double x, y;
    int cluster_id;
} PointData;

typedef struct {
    double x, y;
    int count;
} ClusterCenter;

typedef struct {
    int thread_id;
    int start_idx;
    int end_idx;
    PointData *points;
    ClusterCenter *centers;
    int k;
    atomic_int *changed;
} ThreadArgs;

PointData *global_points = NULL;
int total_points = 0;
int num_threads_max = 0;

double distance(Point p1, Point p2) {
    return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

void* assign_clusters_thread(void* arg) {
    ThreadArgs *args = (ThreadArgs*) arg;

    for (int i = args->start_idx; i < args->end_idx; i++) {
        double min_dist = distance((Point){args->points[i].x, args->points[i].y}, 
                                    (Point){args->centers[0].x, args->centers[0].y});
        int best_cluster = 0;

        for (int j = 1; j < args->k; j++) {
            double dist = distance((Point){args->points[i].x, args->points[i].y},
                                   (Point){args->centers[j].x, args->centers[j].y});
            if (dist < min_dist) {
                min_dist = dist;
                best_cluster = j;
            }
        }

        if (args->points[i].cluster_id != best_cluster) {
            args->points[i].cluster_id = best_cluster;
            atomic_store(args->changed, 1);
        }
    }

    pthread_exit(NULL);
    return NULL;
}

void recalculate_centers(PointData *points, int n, ClusterCenter *centers, int k) {
    for (int i = 0; i < k; i++) {
        centers[i].x = 0;
        centers[i].y = 0;
        centers[i].count = 0;
    }

    for (int i = 0; i < n; i++) {
        int cluster_id = points[i].cluster_id;
        centers[cluster_id].x += points[i].x;
        centers[cluster_id].y += points[i].y;
        centers[cluster_id].count++;
    }

    for (int i = 0; i < k; i++) {
        if (centers[i].count > 0) {
            centers[i].x /= centers[i].count;
            centers[i].y /= centers[i].count;
        }
    }
}

void generate_random_points(PointData *points, int n) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        points[i].x = (double)(rand() % 1000) / 10;
        points[i].y = (double)(rand() % 1000) / 10;
        points[i].cluster_id = 0;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Использование: %s <число_точек> <число_кластеров_K> <макс_потоков>\n", argv[0]);
        return 1;
    }

    total_points = atoi(argv[1]);
    int k = atoi(argv[2]);
    num_threads_max = atoi(argv[3]);

    if (num_threads_max <= 0) num_threads_max = 1;
    if (k <= 0) k = 1;

    global_points = (PointData*)malloc(sizeof(PointData) * total_points);
    ClusterCenter *centers = (ClusterCenter*)malloc(sizeof(ClusterCenter) * k);

    if (!global_points || !centers) {
        fprintf(stderr, "Ошибка выделения памяти\n");
        return 1;
    }

    generate_random_points(global_points, total_points);

    for (int i = 0; i < k; i++) {
        centers[i].x = global_points[i].x;
        centers[i].y = global_points[i].y;
        centers[i].count = 0;
    }

    printf("Запуск k=%d с потоками: %d, точек: %d\n\n", 
           k, num_threads_max, total_points);
    
    int max_iterations = 100;
    atomic_int changed = 1;
    int iter = 0;

    while (atomic_load(&changed) && iter < max_iterations) {
        atomic_store(&changed, 0);
        iter++;

        int points_per_thread = total_points / num_threads_max;
        int remaining_points = total_points % num_threads_max;

        pthread_t threads[num_threads_max];
        ThreadArgs args[num_threads_max];

        int current_idx = 0;
        for (int t = 0; t < num_threads_max; t++) {
            args[t].thread_id = t;
            args[t].start_idx = current_idx;
            
            int extra = (t < remaining_points) ? 1 : 0;
            args[t].end_idx = current_idx + points_per_thread + extra;
            
            args[t].points = global_points;
            args[t].centers = centers;
            args[t].k = k;
            args[t].changed = &changed;

            if (pthread_create(&threads[t], NULL, assign_clusters_thread, (void*)&args[t]) != 0) {
                perror("Ошибка создания потока");
                return 1;
            }

            current_idx = args[t].end_idx;
        }

        for (int t = 0; t < num_threads_max; t++) {
            pthread_join(threads[t], NULL);
        }

        recalculate_centers(global_points, total_points, centers, k);

        printf("Итерация %d завершена, изменения: %s\n", 
               iter, atomic_load(&changed) ? "да" : "нет");
    }


    printf("\n---Результаты---\n");
    printf("Количество итераций: %d\n", iter);
    
    printf("\nТочки:\n");
    for (int i = 0; i < (total_points); i++) {
        printf("Точка %3d: (%.2f, %.2f) -> Кластер %d\n", 
               i, global_points[i].x, global_points[i].y, global_points[i].cluster_id);
    }

    free(global_points);
    free(centers);

    return 0;
}
