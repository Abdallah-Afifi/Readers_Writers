#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>
#include <semaphore.h>
#include <mutex>

// This implementation uses POSIX semaphores for synchronization
class ReadersWriterSemaphore {
private:
    sem_t mutex;           // For mutual exclusion when updating reader_count
    sem_t write_mutex;     // For exclusive writer access
    sem_t read_mutex;      // To block readers when writers are waiting
    int reader_count;      // Number of active readers
    bool writer_waiting;   // Flag to indicate if writers are waiting
    std::mutex print_mutex; // For synchronized console output

public:
    ReadersWriterSemaphore() : reader_count(0), writer_waiting(false) {
        // Initialize semaphores
        sem_init(&mutex, 0, 1);       // Binary semaphore
        sem_init(&write_mutex, 0, 1); // Binary semaphore
        sem_init(&read_mutex, 0, 1);  // Binary semaphore
    }

    ~ReadersWriterSemaphore() {
        // Destroy semaphores
        sem_destroy(&mutex);
        sem_destroy(&write_mutex);
        sem_destroy(&read_mutex);
    }

    // Writer attempts to acquire lock
    void writer_lock() {
        // Signal that a writer is waiting
        sem_wait(&mutex);
        writer_waiting = true;
        sem_post(&mutex);
        
        // Wait for exclusive access to the resource
        sem_wait(&write_mutex);
        
        // Reset writer waiting flag
        sem_wait(&mutex);
        writer_waiting = false;
        sem_post(&mutex);
    }

    // Writer releases lock
    void writer_unlock() {
        sem_post(&write_mutex);
    }

    // Reader attempts to acquire lock
    void reader_lock() {
        // Check if writers are waiting
        sem_wait(&read_mutex);
        
        sem_wait(&mutex);
        bool should_wait = writer_waiting;
        
        if (!should_wait) {
            reader_count++;
            
            // First reader acquires write lock
            if (reader_count == 1) {
                sem_wait(&write_mutex);
            }
        }
        
        sem_post(&mutex);
        sem_post(&read_mutex);
        
        // If there are writers waiting, this reader should wait and try again
        if (should_wait) {
            std::this_thread::yield(); // Give up CPU time
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            reader_lock(); // Try again
        }
    }

    // Reader releases lock
    void reader_unlock() {
        sem_wait(&mutex);
        reader_count--;
        
        // Last reader releases write lock
        if (reader_count == 0) {
            sem_post(&write_mutex);
        }
        
        sem_post(&mutex);
    }

    // Print status with synchronized output
    void print_status(const std::string& message) {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << message << std::endl;
    }
};

// Shared resource (simulated as an integer)
class SharedData {
private:
    int data;
    ReadersWriterSemaphore rwlock;
    
public:
    SharedData() : data(0) {}
    
    // Reader function
    void reader(int id) {
        std::string start_msg = "Reader " + std::to_string(id) + " wants to read.";
        rwlock.print_status(start_msg);
        
        rwlock.reader_lock();
        
        std::string reading_msg = "Reader " + std::to_string(id) + " reading data: " + std::to_string(data);
        rwlock.print_status(reading_msg);
        
        // Simulate reading process
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 900)));
        
        rwlock.reader_unlock();
        
        std::string end_msg = "Reader " + std::to_string(id) + " finished reading.";
        rwlock.print_status(end_msg);
    }
    
    // Writer function
    void writer(int id) {
        std::string start_msg = "Writer " + std::to_string(id) + " wants to write.";
        rwlock.print_status(start_msg);
        
        rwlock.writer_lock();
        
        // Generate a new value and update data
        int new_value = rand() % 1000;
        std::string writing_msg = "Writer " + std::to_string(id) + " writing data: " + std::to_string(new_value);
        rwlock.print_status(writing_msg);
        
        data = new_value;
        
        // Simulate writing process
        std::this_thread::sleep_for(std::chrono::milliseconds(200 + (rand() % 800)));
        
        rwlock.writer_unlock();
        
        std::string end_msg = "Writer " + std::to_string(id) + " finished writing.";
        rwlock.print_status(end_msg);
    }
};

// Statistics for the demonstration
struct Stats {
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
};

int main() {
    // Seed for random number generation
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create shared resource
    SharedData resource;
    Stats stats;
    
    // Create threads for readers and writers
    // Use environment variables if provided, otherwise use defaults
    const int num_readers = std::getenv("READERS") ? std::stoi(std::getenv("READERS")) : 8;
    const int num_writers = std::getenv("WRITERS") ? std::stoi(std::getenv("WRITERS")) : 4;
    const int ops_per_thread = std::getenv("OPERATIONS") ? std::stoi(std::getenv("OPERATIONS")) : 3;
    
    std::cout << "Configuration: " << num_readers << " readers, " << num_writers 
              << " writers, " << ops_per_thread << " operations per thread" << std::endl;
    
    std::cout << "Starting semaphore-based readers-writers demonstration with "
              << num_readers << " readers and " 
              << num_writers << " writers." << std::endl;
    
    std::vector<std::thread> threads;
    
    // Create reader threads
    for (int i = 0; i < num_readers; i++) {
        threads.emplace_back([&resource, &stats, i, ops_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> delay_dist(100, 1000);
            
            for (int j = 0; j < ops_per_thread; j++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
                resource.reader(i + 1);
                stats.total_reads++;
            }
        });
    }
    
    // Create writer threads
    for (int i = 0; i < num_writers; i++) {
        threads.emplace_back([&resource, &stats, i, ops_per_thread]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> delay_dist(200, 1500);
            
            for (int j = 0; j < ops_per_thread; j++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
                resource.writer(i + 1);
                stats.total_writes++;
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    std::cout << "\nDemonstration completed!" << std::endl;
    std::cout << "Final statistics:" << std::endl;
    std::cout << "Total reads: " << stats.total_reads << std::endl;
    std::cout << "Total writes: " << stats.total_writes << std::endl;
    
    return 0;
}
