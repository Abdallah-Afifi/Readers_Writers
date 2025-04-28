#include <iostream>
#include <thread>
#include <shared_mutex>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <cstdlib> // For getenv, stoi

// Implementation of Readers-Writers problem using C++17's std::shared_mutex
// This approach uses the standard library's built-in read-write lock
class ReadersWriterLock {
private:
    std::shared_mutex rwmutex;     // C++17 shared mutex for read-write locks
    std::mutex print_mutex;        // For synchronized console output
    
public:
    // Reader tries to acquire the lock
    void read_lock() {
        rwmutex.lock_shared();
    }
    
    // Reader releases the lock
    void read_unlock() {
        rwmutex.unlock_shared();
    }
    
    // Writer tries to acquire the lock
    void write_lock() {
        rwmutex.lock();
    }
    
    // Writer releases the lock
    void write_unlock() {
        rwmutex.unlock();
    }
    
    // Print status with synchronized output
    void print_status(const std::string& message) {
        std::lock_guard<std::mutex> lock(print_mutex);
        std::cout << message << std::endl;
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
    int reader(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " wants to read." << std::endl;
        }
        
        // Acquire read lock
        auto start_time = std::chrono::steady_clock::now();
        rwlock.read_lock();
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " is reading data: " << data 
                      << " (waited " << wait_time << "ms)" << std::endl;
        }
        
        // Simulate reading process
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 900)));
        
        // Release read lock
        rwlock.read_unlock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " finished reading." << std::endl;
        }
        
        return wait_time;
    }
    
    // Writer function: modifies the shared resource
    int writer(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " wants to write." << std::endl;
        }
        
        // Acquire write lock
        auto start_time = std::chrono::steady_clock::now();
        rwlock.write_lock();
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Simulate writing process
        int new_value = rand() % 1000;
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " is writing data: " << new_value 
                      << " (waited " << wait_time << "ms)" << std::endl;
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
        
        return wait_time;
    }
    
    // Get the current data value
    int get_data() const {
        return data;
    }
};

// Statistics for the demonstration
struct Statistics {
    std::atomic<int> total_reads{0};
    std::atomic<int> total_writes{0};
    std::atomic<int> readers_waiting{0};
    std::atomic<int> writers_waiting{0};
    std::atomic<long long> reader_wait_time{0};
    std::atomic<long long> writer_wait_time{0};
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
    const int operations_per_thread = std::getenv("OPERATIONS") ? std::stoi(std::getenv("OPERATIONS")) : 5;
    
    std::cout << "Configuration: " << num_readers << " readers, " << num_writers 
              << " writers, " << operations_per_thread << " operations per thread" << std::endl;
    
    std::vector<std::thread> threads;
    
    std::cout << "Starting readers-writers demonstration (STD::SHARED_MUTEX) with "
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
            long long wait_time = resource.reader(id);
            stats.readers_waiting--;
            stats.total_reads++;
            stats.reader_wait_time += wait_time;
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
            long long wait_time = resource.writer(id);
            stats.writers_waiting--;
            stats.total_writes++;
            stats.writer_wait_time += wait_time;
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
            
            // Calculate average wait times
            float avg_reader_wait = stats.total_reads > 0 ? 
                                     static_cast<float>(stats.reader_wait_time) / stats.total_reads : 0;
            float avg_writer_wait = stats.total_writes > 0 ? 
                                     static_cast<float>(stats.writer_wait_time) / stats.total_writes : 0;
            
            std::cout << "Avg reader wait time: " << avg_reader_wait << " ms" << std::endl;
            std::cout << "Avg writer wait time: " << avg_writer_wait << " ms" << std::endl;
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
    
    // Calculate final average wait times
    float avg_reader_wait = stats.total_reads > 0 ? 
                             static_cast<float>(stats.reader_wait_time) / stats.total_reads : 0;
    float avg_writer_wait = stats.total_writes > 0 ? 
                             static_cast<float>(stats.writer_wait_time) / stats.total_writes : 0;
    
    std::cout << "Avg reader wait time: " << avg_reader_wait << " ms" << std::endl;
    std::cout << "Avg writer wait time: " << avg_writer_wait << " ms" << std::endl;
    
    return 0;
}
