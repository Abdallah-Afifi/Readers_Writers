# Makefile for Readers-Writers Problem

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread

# Original implementations
TARGET_WRITERS_PRIORITY = readers_writers
TARGET_SEMAPHORE = readers_writers_semaphore

# New implementations
TARGET_READERS_PRIORITY = readers_writers_readers_priority
TARGET_FAIR = readers_writers_fair
TARGET_SHARED_MUTEX = readers_writers_shared_mutex
TARGET_MONITOR = readers_writers_monitor
TARGET_EDUCATIONAL = readers_writers_educational

# All targets
TARGETS = $(TARGET_WRITERS_PRIORITY) $(TARGET_SEMAPHORE) \
          $(TARGET_READERS_PRIORITY) $(TARGET_FAIR) \
          $(TARGET_SHARED_MUTEX) $(TARGET_MONITOR) \
          $(TARGET_EDUCATIONAL)

all: $(TARGETS)

# Original implementations
$(TARGET_WRITERS_PRIORITY): readers_writers.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET_SEMAPHORE): readers_writers_semaphore.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# New implementations
$(TARGET_READERS_PRIORITY): readers_writers_readers_priority.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET_FAIR): readers_writers_fair.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET_SHARED_MUTEX): readers_writers_shared_mutex.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET_MONITOR): readers_writers_monitor.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(TARGET_EDUCATIONAL): readers_writers_educational.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)

# Individual run targets
run_writers_priority: $(TARGET_WRITERS_PRIORITY)
	./$(TARGET_WRITERS_PRIORITY)

run_semaphore: $(TARGET_SEMAPHORE)
	./$(TARGET_SEMAPHORE)

run_readers_priority: $(TARGET_READERS_PRIORITY)
	./$(TARGET_READERS_PRIORITY)

run_fair: $(TARGET_FAIR)
	./$(TARGET_FAIR)

run_shared_mutex: $(TARGET_SHARED_MUTEX)
	./$(TARGET_SHARED_MUTEX)

run_monitor: $(TARGET_MONITOR)
	./$(TARGET_MONITOR)

run_educational: $(TARGET_EDUCATIONAL)
	./$(TARGET_EDUCATIONAL)

# Run all implementations
run_all: $(TARGETS)
	./readers_writers_demo.sh

# Run benchmark mode
benchmark: $(TARGETS)
	./readers_writers_demo.sh --benchmark

# Quick demonstration mode
quick: $(TARGETS)
	./readers_writers_demo.sh --quick

# Verbose mode
verbose: $(TARGETS)
	./readers_writers_demo.sh --verbose

# Custom run configurations
run_custom_small: $(TARGETS)
	READERS=8 WRITERS=3 OPERATIONS=3 ./readers_writers_demo.sh

run_custom_large: $(TARGETS)
	READERS=20 WRITERS=10 OPERATIONS=5 ./readers_writers_demo.sh

# Show documentation information
docs:
	@echo "Documentation files available:"
	@echo "- Report.md: Comprehensive project report with all implementation details"
	@echo "- README.md: Installation and usage guide"

# Help target
help:
	@echo "Readers-Writers Problem - Available targets:"
	@echo ""
	@echo "Main targets:"
	@echo "  make               Build all implementations"
	@echo "  make run_all       Run all implementations with default settings"
	@echo "  make benchmark     Run benchmark mode for all implementations"
	@echo "  make quick         Run quick demonstration mode"
	@echo "  make verbose       Run with verbose output"
	@echo "  make clean         Remove compiled binaries"
	@echo ""
	@echo "Individual implementations:"
	@echo "  make run_writers_priority    Run writers-priority implementation"
	@echo "  make run_readers_priority    Run readers-priority implementation"
	@echo "  make run_fair                Run fair/queue-based implementation"
	@echo "  make run_semaphore           Run semaphore-based implementation"
	@echo "  make run_shared_mutex        Run std::shared_mutex implementation"
	@echo "  make run_monitor             Run monitor-based implementation"
	@echo "  make run_educational         Run educational implementation"
	@echo ""
	@echo "Configuration variants:"
	@echo "  make run_custom_small        Run with 8 readers, 3 writers"
	@echo "  make run_custom_large        Run with 20 readers, 10 writers"
	@echo ""
	@echo "Documentation:"
	@echo "  make docs                    List available documentation files"
	@echo ""
	@echo "Note: All functionality is available through the single 'readers_writers_demo.sh' script"

.PHONY: all clean run_writers_priority run_semaphore run_readers_priority \
        run_fair run_shared_mutex run_monitor run_educational run_all benchmark \
        quick verbose run_custom_small run_custom_large docs help
