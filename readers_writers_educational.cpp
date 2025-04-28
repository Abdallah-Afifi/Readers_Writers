/**
 * readers_writers_educational.cpp - Educational implementation of Readers-Writers problem
 * 
 * This file contains a simplified, heavily-commented implementation of the Readers-Writers
 * problem that's designed for educational purposes. Comments explain the concepts,
 * challenges, and solutions used.
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <cstdlib>

/**
 * ReadersWriterLock - A synchronization mechanism that implements the readers-writer pattern
 * 
 * This class demonstrates a classic solution to the readers-writer problem, providing:
 * 1. Multiple simultaneous reader access when no writer is active
 * 2. Exclusive writer access when no readers or other writers are active
 * 3. Writer preference to prevent writer starvation
 */
class ReadersWriterLock {
private:
    // SYNCHRONIZATION PRIMITIVES
    std::mutex mtx;                // Protects shared state (reader count and writer flags)
    std::condition_variable cv;    // For threads to wait on state changes

    // SHARED STATE (protected by mtx)
    int reader_count = 0;          // Number of active readers
    bool writer_active = false;    // Flag indicating if a writer is active
    int waiting_writers = 0;       // Number of writers waiting for access
    
public:
    /**
     * read_lock() - Acquire read access to the protected resource
     * 
     * This function will block in two cases:
     * 1. A writer is currently active
     * 2. Writers are waiting (writer preference)
     */
    void read_lock() {
        // The unique_lock provides RAII-style locking of the mutex
        // It locks the mutex when constructed and unlocks when destructed
        std::unique_lock<std::mutex> lock(mtx);
        
        // KEY INSIGHT: Writers have preference to prevent starvation
        // Readers wait if:
        // - A writer is currently active, OR
        // - There are writers waiting (even if no writer is active)
        cv.wait(lock, [this] { 
            return !writer_active && waiting_writers == 0; 
        });
        
        // Increment the reader count to track active readers
        reader_count++;
        
        // Note: We keep the mutex locked only for the critical section of
        // updating the reader count, not during the actual reading
        // lock will automatically release when it goes out of scope
    }
    
    /**
     * read_unlock() - Release read access to the protected resource
     * 
     * This function updates the reader count and notifies waiting writers
     * when the last reader exits.
     */
    void read_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Decrement reader count and check if we're the last reader
        reader_count--;
        
        // If we're the last reader and writers are waiting,
        // signal that writers can proceed
        if (reader_count == 0 && waiting_writers > 0) {
            cv.notify_all();  // Could use notify_one() but notify_all() is safer
        }
        
        // lock automatically releases when it goes out of scope
    }
    
    /**
     * write_lock() - Acquire exclusive write access to the protected resource
     * 
     * This function will block until:
     * 1. No reader is active
     * 2. No writer is active
     */
    void write_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Increment the waiting writers count to signal readers to wait
        waiting_writers++;
        
        // KEY INSIGHT: Writers need to wait for two conditions:
        // 1. No readers are active
        // 2. No other writer is active
        cv.wait(lock, [this] { 
            return reader_count == 0 && !writer_active; 
        });
        
        // We're no longer waiting since we're about to become active
        waiting_writers--;
        
        // Mark that a writer is now active
        writer_active = true;
        
        // lock releases automatically when it goes out of scope
    }
    
    /**
     * write_unlock() - Release exclusive write access to the protected resource
     * 
     * This function marks the writer as inactive and notifies waiting threads.
     */
    void write_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Mark writer as no longer active
        writer_active = false;
        
        // Notify all waiting threads that the writer is done
        // If there are waiting writers, only they will proceed (due to the wait condition)
        // If no writers are waiting, readers will proceed
        cv.notify_all();
        
        // lock automatically releases when it goes out of scope
    }
};

/**
 * SharedResource - The protected resource being accessed by readers and writers
 * 
 * This class represents the resource that multiple readers can access simultaneously,
 * but writers need exclusive access to.
 */
class SharedResource {
private:
    int data = 0;                      // The actual data being protected
    ReadersWriterLock rwlock;          // The synchronization mechanism
    std::mutex print_mutex;            // For synchronized console output
    
public:
    /**
     * reader() - Simulates a reader accessing the shared resource
     * 
     * @param id - The reader's identifier
     * @return long long - The time the reader waited for access
     */
    long long reader(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " wants to read." << std::endl;
        }
        
        // Measure how long the reader waits to acquire the lock
        auto start_time = std::chrono::steady_clock::now();
        
        // Acquire read lock
        rwlock.read_lock();
        
        // Calculate wait time
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Critical section: access the shared resource
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " is reading data: " << data 
                      << " (waited " << wait_time << "ms)" << std::endl;
        }
        
        // Simulate time spent reading
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 500)));
        
        // Release read lock
        rwlock.read_unlock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " finished reading." << std::endl;
        }
        
        return wait_time;
    }
    
    /**
     * writer() - Simulates a writer modifying the shared resource
     * 
     * @param id - The writer's identifier
     * @return long long - The time the writer waited for access
     */
    long long writer(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " wants to write." << std::endl;
        }
        
        // Measure how long the writer waits to acquire the lock
        auto start_time = std::chrono::steady_clock::now();
        
        // Acquire write lock
        rwlock.write_lock();
        
        // Calculate wait time
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Critical section: modify the shared resource
        // Generate a new random value
        int new_value = rand() % 1000;
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " is writing data: " << new_value 
                      << " (waited " << wait_time << "ms)" << std::endl;
        }
        
        // Update the data
        data = new_value;
        
        // Simulate time spent writing
        std::this_thread::sleep_for(std::chrono::milliseconds(200 + (rand() % 500)));
        
        // Release write lock
        rwlock.write_unlock();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " finished writing." << std::endl;
        }
        
        return wait_time;
    }
};

// Statistics for the demonstration
struct Statistics {
    std::atomic<int> total_reads{0};       // Count of completed read operations
    std::atomic<int> total_writes{0};      // Count of completed write operations
    std::atomic<int> readers_waiting{0};   // Count of readers currently waiting
    std::atomic<int> writers_waiting{0};   // Count of writers currently waiting
    std::atomic<long long> reader_wait_time{0}; // Total wait time for all readers
    std::atomic<long long> writer_wait_time{0}; // Total wait time for all writers
};

int main() {
    // Seed for random number generation
    srand(static_cast<unsigned int>(time(nullptr)));
    
    // Create shared resource
    SharedResource resource;
    Statistics stats;
    
    // Create threads for readers and writers
    // Use environment variables if provided, otherwise use defaults
    const int num_readers = std::getenv("READERS") ? std::stoi(std::getenv("READERS")) : 8;
    const int num_writers = std::getenv("WRITERS") ? std::stoi(std::getenv("WRITERS")) : 4;
    const int operations_per_thread = std::getenv("OPERATIONS") ? std::stoi(std::getenv("OPERATIONS")) : 3;
    
    std::cout << "Educational Readers-Writers Demonstration:" << std::endl;
    std::cout << "Configuration: " << num_readers << " readers, " << num_writers 
              << " writers, " << operations_per_thread << " operations per thread" << std::endl;
    
    std::vector<std::thread> threads;
    
    // Lambda to simulate reader behavior with random intervals
    auto reader_task = [&resource, &stats, operations_per_thread](int id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> delay_dist(100, 1000);  // 100-1000ms delay
        
        for (int i = 0; i < operations_per_thread; i++) {
            // Random delay before attempting to read
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
            
            // Track waiting readers
            stats.readers_waiting++;
            
            // Perform the read operation and get wait time
            long long wait_time = resource.reader(id);
            
            // Update statistics
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
            
            // Track waiting writers
            stats.writers_waiting++;
            
            // Perform the write operation and get wait time
            long long wait_time = resource.writer(id);
            
            // Update statistics
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
        
        // Continue monitoring until all operations complete
        while (total_operations < expected_operations) {
            // Update every 2 seconds
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Get current operation count
            total_operations = stats.total_reads + stats.total_writes;
            
            // Display current statistics
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
    
    // Join the monitor thread
    if (monitor.joinable()) {
        monitor.join();
    }
    
    // Display final statistics
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
