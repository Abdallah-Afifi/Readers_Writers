#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <random>
#include <atomic>
#include <cstdlib> // For getenv, stoi

// Implementation of Readers-Writers problem using a monitor approach
// A monitor encapsulates shared data with procedures that provide synchronized access
class ReadersWriterMonitor {
private:
    std::mutex monitor_mutex;           // The monitor lock
    std::condition_variable read_cv;    // Condition variable for readers
    std::condition_variable write_cv;   // Condition variable for writers
    
    int reader_count = 0;               // Number of active readers
    bool writer_active = false;         // Flag to check if writer is active
    int waiting_readers = 0;            // Number of waiting readers
    int waiting_writers = 0;            // Number of waiting writers
    
public:
    // Reader tries to enter the monitor
    void start_read() {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        
        // Increment waiting readers count
        waiting_readers++;
        
        // Wait if there's an active writer or waiting writers (writer preference)
        while (writer_active || waiting_writers > 0) {
            read_cv.wait(lock);
        }
        
        // Decrement waiting readers and increment active readers
        waiting_readers--;
        reader_count++;
        
        // Signal other waiting readers that they can proceed too
        read_cv.notify_one();
        
        lock.unlock();
    }
    
    // Reader finishes reading
    void end_read() {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        
        // Decrement active readers count
        reader_count--;
        
        // If this was the last reader and writers are waiting, signal a writer
        if (reader_count == 0 && waiting_writers > 0) {
            write_cv.notify_one();
        }
        
        lock.unlock();
    }
    
    // Writer tries to enter the monitor
    void start_write() {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        
        // Increment waiting writers count
        waiting_writers++;
        
        // Wait until there are no active readers and no active writers
        while (reader_count > 0 || writer_active) {
            write_cv.wait(lock);
        }
        
        // Decrement waiting writers count and mark writer as active
        waiting_writers--;
        writer_active = true;
        
        lock.unlock();
    }
    
    // Writer finishes writing
    void end_write() {
        std::unique_lock<std::mutex> lock(monitor_mutex);
        
        // Mark writer as inactive
        writer_active = false;
        
        // If there are waiting writers, signal one writer
        if (waiting_writers > 0) {
            write_cv.notify_one();
        } 
        // Otherwise signal all waiting readers
        else if (waiting_readers > 0) {
            read_cv.notify_all();
        }
        
        lock.unlock();
    }
    
    // Get monitor state for diagnostics
    void get_state(int& readers, int& writers, int& w_readers, int& w_writers) const {
        // Use const_cast since this is just for diagnostics and doesn't modify the state
        std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&monitor_mutex));
        readers = reader_count;
        writers = writer_active ? 1 : 0;
        w_readers = waiting_readers;
        w_writers = waiting_writers;
    }
};

// Shared resource (simulated as an integer)
class SharedResource {
private:
    int data = 0;
    ReadersWriterMonitor monitor;
    std::mutex print_mutex;  // For synchronized console output
    
public:
    // Reader function: reads data from the shared resource
    int reader(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " wants to read." << std::endl;
        }
        
        // Enter monitor to read
        auto start_time = std::chrono::steady_clock::now();
        monitor.start_read();
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Critical section - reading data
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " is reading data: " << data 
                      << " (waited " << wait_time << "ms)" << std::endl;
        }
        
        // Simulate reading process
        std::this_thread::sleep_for(std::chrono::milliseconds(100 + (rand() % 900)));
        
        // Exit monitor
        monitor.end_read();
        
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
        
        // Enter monitor to write
        auto start_time = std::chrono::steady_clock::now();
        monitor.start_write();
        auto end_time = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        // Generate new data value
        int new_value = rand() % 1000;
        
        // Critical section - writing data
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " is writing data: " << new_value 
                      << " (waited " << wait_time << "ms)" << std::endl;
        }
        
        // Modify the shared data
        data = new_value;
        
        // Simulate additional processing
        std::this_thread::sleep_for(std::chrono::milliseconds(200 + (rand() % 800)));
        
        // Exit monitor
        monitor.end_write();
        
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Writer " << id << " finished writing." << std::endl;
        }
        
        return wait_time;
    }
    
    // Get monitor state
    void get_monitor_state(int& readers, int& writers, int& w_readers, int& w_writers) const {
        monitor.get_state(readers, writers, w_readers, w_writers);
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
    
    std::cout << "Starting readers-writers demonstration (MONITOR-BASED) with "
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
    std::thread monitor([&resource, &stats, num_readers, num_writers, operations_per_thread]() {
        int expected_operations = (num_readers + num_writers) * operations_per_thread;
        int total_operations = 0;
        
        while (total_operations < expected_operations) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            total_operations = stats.total_reads + stats.total_writes;
            
            // Get monitor state
            int active_readers, active_writers, waiting_readers, waiting_writers;
            resource.get_monitor_state(active_readers, active_writers, waiting_readers, waiting_writers);
            
            std::cout << "\n----- STATISTICS -----" << std::endl;
            std::cout << "Completed reads: " << stats.total_reads << std::endl;
            std::cout << "Completed writes: " << stats.total_writes << std::endl;
            std::cout << "Active readers: " << active_readers << std::endl;
            std::cout << "Active writers: " << active_writers << std::endl;
            std::cout << "Monitor waiting readers: " << waiting_readers << std::endl;
            std::cout << "Monitor waiting writers: " << waiting_writers << std::endl;
            std::cout << "Threads waiting to read: " << stats.readers_waiting << std::endl;
            std::cout << "Threads waiting to write: " << stats.writers_waiting << std::endl;
            
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
