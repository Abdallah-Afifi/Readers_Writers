#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <vector>
#include <queue>
#include <random>
#include <atomic>
#include <memory>
#include <cstdlib> // For getenv, stoi

enum class RequestType { READ, WRITE };

// A fair implementation of Readers-Writers problem that prevents starvation
// Uses a FIFO queue to ensure fair access for both readers and writers
class FairReadersWriterLock {
private:
    struct Request {
        RequestType type;
        std::shared_ptr<std::condition_variable> cv;
        bool granted = false;
        
        Request(RequestType t) : type(t), cv(std::make_shared<std::condition_variable>()) {}
    };

    std::mutex mtx;                // Protects access to shared data
    std::mutex resource_mutex;     // Provides exclusive access to the resource
    std::queue<std::shared_ptr<Request>> request_queue; // Queue of pending requests
    
    int active_readers = 0;        // Number of active readers
    bool writer_active = false;    // Flag to check if writer is active
    
    // Process the request queue to grant access when possible
    void process_queue() {
        if (request_queue.empty()) return;

        auto request = request_queue.front();
        
        if (request->type == RequestType::READ) {
            // Grant read access if no writer is active
            if (!writer_active) {
                request_queue.pop();
                active_readers++;
                request->granted = true;
                request->cv->notify_one();
                
                // Process additional read requests that can be granted simultaneously
                while (!request_queue.empty() && request_queue.front()->type == RequestType::READ) {
                    auto next_read = request_queue.front();
                    request_queue.pop();
                    active_readers++;
                    next_read->granted = true;
                    next_read->cv->notify_one();
                }
            }
        } else { // RequestType::WRITE
            // Grant write access if no readers or writers are active
            if (active_readers == 0 && !writer_active) {
                request_queue.pop();
                writer_active = true;
                request->granted = true;
                request->cv->notify_one();
            }
        }
    }
    
public:
    // Reader tries to acquire the lock
    void read_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Create a read request
        auto request = std::make_shared<Request>(RequestType::READ);
        request_queue.push(request);
        
        // Try to process the queue (may grant this request immediately)
        process_queue();
        
        // Wait if request not granted yet
        while (!request->granted) {
            request->cv->wait(lock);
        }
        
        // Note: First reader has already acquired the resource mutex in process_queue
        lock.unlock();
    }
    
    // Reader releases the lock
    void read_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Decrement the reader count
        active_readers--;
        
        // Last reader releases the resource lock
        if (active_readers == 0) {
            resource_mutex.unlock();
        }
        
        // Process the next request in the queue
        process_queue();
        
        lock.unlock();
    }
    
    // Writer tries to acquire the lock
    void write_lock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Create a write request
        auto request = std::make_shared<Request>(RequestType::WRITE);
        request_queue.push(request);
        
        // Try to process the queue (may grant this request immediately)
        process_queue();
        
        // Wait if request not granted yet
        while (!request->granted) {
            request->cv->wait(lock);
        }
        
        lock.unlock();
        
        // Acquire exclusive access to the resource
        resource_mutex.lock();
    }
    
    // Writer releases the lock
    void write_unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        
        // Mark writer as inactive
        writer_active = false;
        
        // Release exclusive access to the resource
        resource_mutex.unlock();
        
        // Process the next request in the queue
        process_queue();
        
        lock.unlock();
    }
    
    // Get queue size for monitoring
    size_t queue_size() const {
        // Use const_cast since this is just for monitoring and doesn't modify the queue
        std::lock_guard<std::mutex> lock(*const_cast<std::mutex*>(&mtx));
        return request_queue.size();
    }
};

// Shared resource (simulated as an integer)
class SharedResource {
private:
    int data = 0;
    FairReadersWriterLock rwlock;
    std::mutex print_mutex;  // For synchronized console output
    
public:
    // Reader function: reads data from the shared resource
    int reader(int id) {
        {
            std::lock_guard<std::mutex> print_lock(print_mutex);
            std::cout << "Reader " << id << " wants to read (queue size: " 
                      << rwlock.queue_size() << ")." << std::endl;
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
            std::cout << "Writer " << id << " wants to write (queue size: " 
                      << rwlock.queue_size() << ")." << std::endl;
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
    
    std::cout << "Starting readers-writers demonstration (FAIR/STARVATION-FREE) with "
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
