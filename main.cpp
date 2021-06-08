#include <iostream>
#include <vector>
#include <string>
#include <utility>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include <random>

#define WRITER_COUNT 25
#define READER_COUNT 15
#define ITERATION_COUNT 10
#define DELAY 500 // В микросекундах (10^(-6) с.)

void *writer_creator(void *);
void *reader_creator(void *);
void *writer_function(void *);
void *reader_function(void *);

std::pair<pthread_mutex_t, pthread_mutex_t> utility_mutexes = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
std::pair<pthread_mutex_t, pthread_mutex_t> role_mutexes    = {PTHREAD_MUTEX_INITIALIZER, PTHREAD_MUTEX_INITIALIZER};
std::pair<int, int>                         role_counters   = {0, 0};

std::random_device randomizer_seed;
std::mt19937 distribution_seed(randomizer_seed());
std::uniform_int_distribution<int> distribution(0, 1000);

int main() {
    const std::string filename = "../sometext.txt";

    //Чищу файл, если в нём что-то было
    std::ofstream file(filename);
    file.close();

    for (size_t i = 0; i < ITERATION_COUNT; i++)
    {
        std::cout << "Iteration No." << i + 1 << std::endl;
        
        pthread_t w_creator, r_creator;
        //Параллельно создаю потоки-читатели и потоки-писатели 
        pthread_create(&w_creator, NULL, writer_creator, (void *) &filename);
        pthread_create(&r_creator, NULL, reader_creator, (void *) &filename);
        pthread_join(w_creator, NULL);
        pthread_join(r_creator, NULL);
        
        std::cout << std::endl;
    }
    pthread_mutex_destroy(&utility_mutexes.first);
    pthread_mutex_destroy(&utility_mutexes.second);
    pthread_mutex_destroy(&role_mutexes.first);
    pthread_mutex_destroy(&role_mutexes.second);
    
    return 0;
}

void *writer_creator(void *filename) {
    std::vector<pthread_t> writer_threads(WRITER_COUNT);
    for (auto &thread : writer_threads)
    {
        pthread_create(&thread, NULL, writer_function, filename);
        usleep(DELAY);
    }
    for (auto &thread : writer_threads)
    {
        pthread_join(thread, NULL);
    }
    pthread_exit(NULL);
}

void *reader_creator(void *filename) {
    std::vector<pthread_t> reader_threads(WRITER_COUNT);
    for (auto &thread : reader_threads)
    {
        pthread_create(&thread, NULL, reader_function, filename);
        usleep(DELAY);
    }
    for (auto &thread : reader_threads)
    {
        pthread_join(thread, NULL);
    }
    pthread_exit(NULL);
}

// Поток писатель пишет в конец файла случайное число от 0 до 1000
void *writer_function(void *filename) {
    pthread_mutex_lock(&utility_mutexes.first);
    role_counters.first++;
    if(role_counters.first == 1) {
        pthread_mutex_lock(&role_mutexes.second);
    }
    pthread_mutex_unlock(&utility_mutexes.first);

    pthread_mutex_lock(&role_mutexes.first);
    std::ofstream file(*((std::string*)filename), std::ios::app);
    int number = distribution(distribution_seed);
    file << number << std::endl;
    std::cout << "Writer thread added number " << number << " in " << *((std::string*)filename) << std::endl; 
    pthread_mutex_unlock(&role_mutexes.first);

    pthread_mutex_lock(&utility_mutexes.first);
    role_counters.first--;
    if(role_counters.first == 0) {
        pthread_mutex_unlock(&role_mutexes.second);
    }
    pthread_mutex_unlock(&utility_mutexes.first);

    pthread_exit(NULL);
}

// Поток читатель считает сумму чисел из файла и выводит её в стандартный поток вывода
void *reader_function(void *filename) {
    pthread_mutex_lock(&role_mutexes.second);
    pthread_mutex_lock(&utility_mutexes.second);
    role_counters.second++;
    if (role_counters.second == 1) {
        pthread_mutex_lock(&role_mutexes.first);
    }
    pthread_mutex_unlock(&utility_mutexes.second);
    pthread_mutex_unlock(&role_mutexes.second);
    
    int sum = 0;
    int number;
    std::ifstream file(*((std::string*)filename));
    while (file >> number) {
        sum += number;
    }
    std::cout << "Reader thread calculated the sum of numbers from " << *((std::string*)filename) << ": " << sum << std::endl;

    pthread_mutex_lock(&utility_mutexes.second);
    role_counters.second--;
    if (role_counters.second == 0) {
        pthread_mutex_unlock(&role_mutexes.first);
    }
    pthread_mutex_unlock(&utility_mutexes.second);

    pthread_exit(NULL);
}