#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>

class ReadersWriterLock {
private:
    std::mutex mtx;                // Protects access to reader_count
    std::mutex resource_mutex;     // Provides exclusive access to the resource
    std::condition_variable write_cv; // Condition variable for writers
    
    int reader_count = 0;          // Number of active readers
    bool writer_active = false;    // Flag to check if writer is active
    int waiting_writers = 0;       // Number of waiting writers
    
public:
    // Reader tries to acquire the lock
    void read_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // If there's an active writer or waiting writers, readers should wait
        // This gives priority to writers to prevent their starvation
        write_cv.wait(lock, [this] { 
            return !writer_active && waiting_writers == 0; 
        });
        
        // Increment the reader count
        reader_count++;
        
        // First reader acquires the resource lock
        if (reader_count == 1) {
            resource_mutex.lock();
        }
        
        lock.unlock();
    }
    
    // Reader releases the lock
    void read_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Decrement the reader count
        reader_count--;
        
        // Last reader releases the resource lock
        if (reader_count == 0) {
            resource_mutex.unlock();
            // Notify waiting writers that the resource is free
            write_cv.notify_all();
        }
        
        lock.unlock();
    }
    
    // Writer tries to acquire the lock
    void write_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Increment waiting writers count
        waiting_writers++;
        
        // Wait until there are no active readers and no active writers
        write_cv.wait(lock, [this] { 
            return reader_count == 0 && !writer_active; 
        });
        
        // Mark writer as active and decrement waiting count
        writer_active = true;
        waiting_writers--;
        
        lock.unlock();
        
        // Acquire exclusive access to the resource
        resource_mutex.lock();
    }
    
    // Writer releases the lock
    void write_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Mark writer as inactive
        writer_active = false;
        
        lock.unlock();
        
        // Release exclusive access to the resource
        resource_mutex.unlock();
        
        // Notify waiting threads
        write_cv.notify_all();
    }
};

// Shared resource (simulated as an integer)
class SharedResource {
private:
    int data = 0;
    ReadersWriterLock rwlock;
    std::mutex print_mutex;  // For synchronized console output
    
public:
    // Reader function: reads data from the shared resource
    void reader(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " wants to read." << std::endl;
        }
        
        // Acquire read lock
        rwlock.read_lock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " is reading data: " << data << std::endl;
        }
        
        // Simulate reading process
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 900)));
        
        // Release read lock
        rwlock.read_unlock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " finished reading." << std::endl;
        }
    }
    
    // Writer function: modifies the shared resource
    void writer(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " wants to write." << std::endl;
        }
        
        // Acquire write lock
        rwlock.write_lock();
        
        // Simulate writing process
        int new_value = rand() % 1000;
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " is writing data: " << new_value << std::endl;
        }
        
        // Modify the shared data
        data = new_value;
        
        // Simulate additional processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200 + (rand() % 800)));
        
        // Release write lock
        rwlock.write_unlock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " finished writing." << std::endl;
        }
    }
};

// Statistics for the demonstration
struct Statistics {
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
    std::atomic<int> readers_waiting{0};
    std::atomic<int> writers_waiting{0};
};

int main() {
    // Seed for random number generation
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create shared resource
    SharedResource resource;
    Statistics stats;
    
    // Create threads for readers and writers
    // Use environment variables if provided, otherwise use defaults
    const int num_readers = std::getenv("READERS") ? std::stoi(std::getenv("READERS")) : 10;
    const int num_writers = std::getenv("WRITERS") ? std::stoi(std::getenv("WRITERS")) : 5;
    const int operations_per_thread = std::getenv("OPERATIONS") ? std::stoi(std::getenv("OPERATIONS")) : 3;
    
    std::cout << "Configuration: " << num_readers << " readers, " << num_writers 
              << " writers, " << operations_per_thread << " operations per thread" << std::endl;
    
    std::vector<std::thread> threads;
    
    std::cout << "Starting readers-writers demonstration with "
              << num_readers << " readers and " 
              << num_writers << " writers." << std::endl;
    
    // Lambda to simulate reader behavior with random intervals
    auto reader_task = [&resource, &stats, operations_per_thread](int id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay_dist(100, 1000);  // 100-1000ms delay
        
        for (int i = 0; i < operations_per_thread; i++) {
            // Random delay before attempting to read
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            
            stats.readers_waiting++;
            resource.reader(id);
            stats.readers_waiting--;
            stats.total_reads++;
        }
    };
    
    // Lambda to simulate writer behavior with random intervals
    auto writer_task = [&resource, &stats, operations_per_thread](int id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay_dist(200, 1500);  // 200-1500ms delay
        
        for (int i = 0; i < operations_per_thread; i++) {
            // Random delay before attempting to write
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            
            stats.writers_waiting++;
            resource.writer(id);
            stats.writers_waiting--;
            stats.total_writes++;
        }
    };
    
    // Start reader threads
    for (int i = 0; i < num_readers; i++) {
        threads.emplace_back(reader_task, i + 1);
    }
    
    // Start writer threads
    for (int i = 0; i < num_writers; i++) {
        threads.emplace_back(writer_task, i + 1);
    }
    
    // Monitor thread for displaying statistics
    std::thread monitor([&stats, num_readers, num_writers, operations_per_thread]() {
        int expected_operations = (num_readers + num_writers) * operations_per_thread;
        int total_operations = 0;
        
        while (total_operations < expected_operations) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            total_operations = stats.total_reads + stats.total_writes;
            
            std::cout << "\n----- STATISTICS -----" << std::endl;
            std::cout << "Completed reads: " << stats.total_reads << std::endl;
            std::cout << "Completed writes: " << stats.total_writes << std::endl;
            std::cout << "Readers waiting: " << stats.readers_waiting << std::endl;
            std::cout << "Writers waiting: " << stats.writers_waiting << std::endl;
            std::cout << "Progress: " << (total_operations * 100) / expected_operations << "%" << std::endl;
        }
    });
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    if (monitor.joinable()) {
        monitor.join();
    }
    
    std::cout << "\nDemonstration completed!" << std::endl;
    std::cout << "Final statistics:" << std::endl;
    std::cout << "Total reads: " << stats.total_reads << std::endl;
    std::cout << "Total writes: " << stats.total_writes << std::endl;
    
    return 0;
}
