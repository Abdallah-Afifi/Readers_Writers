#!/bin/bash
# readers_writers_demo.sh - A comprehensive demonstration and testing script
# for the Readers-Writers problem implementations

set -e  # Exit on first error

# ANSI color codes for pretty output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
GRAY='\033[0;37m'
BOLD='\033[1m'
RESET='\033[0m'

# Default configuration
TIME_LIMIT=${TIME_LIMIT:-15}  # Default time limit in seconds
READERS=${READERS:-8}         # Default number of readers
WRITERS=${WRITERS:-4}         # Default number of writers
OPERATIONS=${OPERATIONS:-5}   # Default operations per thread

# Command line argument parsing
BENCHMARK=false
QUICK=false
VERBOSE=false

print_help() {
    echo -e "${BOLD}Readers-Writers Problem Demonstration${RESET}"
    echo ""
    echo "Usage: ./readers_writers_demo.sh [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --benchmark    Run in benchmark mode (collect performance metrics)"
    echo "  --quick        Run a shortened version of the demo"
    echo "  --verbose      Show more detailed output"
    echo "  --help         Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  READERS=n      Set number of reader threads (default: 8)"
    echo "  WRITERS=n      Set number of writer threads (default: 4)"
    echo "  OPERATIONS=n   Set operations per thread (default: 5)"
    echo "  TIME_LIMIT=n   Set time limit in seconds (default: 15)"
    echo ""
    echo "Examples:"
    echo "  ./readers_writers_demo.sh"
    echo "  READERS=20 WRITERS=10 ./readers_writers_demo.sh --benchmark"
    echo "  TIME_LIMIT=5 ./readers_writers_demo.sh --quick"
}

# Parse command line arguments
for arg in "$@"
do
    case $arg in
        --benchmark)
        BENCHMARK=true
        shift
        ;;
        --quick)
        QUICK=true
        TIME_LIMIT=5
        shift
        ;;
        --verbose)
        VERBOSE=true
        shift
        ;;
        --help)
        print_help
        exit 0
        ;;
        *)
        # Unknown option
        echo -e "${RED}Unknown option: $arg${RESET}"
        print_help
        exit 1
        ;;
    esac
done

# Print configuration
echo -e "${BOLD}======================================================${RESET}"
echo -e "${BOLD}Readers-Writers Problem Demonstration${RESET}"
echo -e "${BOLD}======================================================${RESET}"
echo ""

echo -e "${BLUE}Configuration:${RESET}"
echo "  - Readers: $READERS"
echo "  - Writers: $WRITERS"
echo "  - Operations per thread: $OPERATIONS"
echo "  - Time limit: $TIME_LIMIT seconds"
if [ "$BENCHMARK" = true ]; then
    echo "  - Mode: Benchmark"
elif [ "$QUICK" = true ]; then
    echo "  - Mode: Quick demo"
else
    echo "  - Mode: Full demonstration"
fi
echo ""

# Compile all implementations
echo -e "${YELLOW}Step 1: Compiling all implementations...${RESET}"
make clean >/dev/null 2>&1 && make >/dev/null 2>&1

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${RESET}"
    exit 1
fi

echo -e "${GREEN}Compilation successful!${RESET}"
echo ""

# Arrays of implementation info
IMPLEMENTATIONS=("readers_writers" "readers_writers_semaphore" "readers_writers_readers_priority" "readers_writers_fair" "readers_writers_shared_mutex" "readers_writers_monitor" "readers_writers_educational")
DESCRIPTIONS=("Writers-Priority" "Semaphore-based" "Readers-Priority" "Fair/Queue-based" "std::shared_mutex" "Monitor-based" "Educational")
COLORS=("$BLUE" "$GREEN" "$YELLOW" "$MAGENTA" "$CYAN" "$RED" "$GRAY")

# Set up for benchmark mode
if [ "$BENCHMARK" = true ]; then
    # Create results directory if it doesn't exist
    mkdir -p results
    
    # Create a CSV file for results
    RESULTS_FILE="results/benchmark_$(date +%Y%m%d_%H%M%S).csv"
    echo "Implementation,Readers,Writers,Operations,Total Reads,Total Writes,Reader Wait Time (ms),Writer Wait Time (ms)" > "$RESULTS_FILE"
    echo -e "${YELLOW}Results will be saved to: ${RESET}$RESULTS_FILE"
    echo ""
fi

# Function to run a single implementation
run_implementation() {
    local index=$1
    local impl=${IMPLEMENTATIONS[$index]}
    local desc=${DESCRIPTIONS[$index]}
    local color=${COLORS[$index]}
    
    if [ "$QUICK" = true ] && [ "$impl" = "readers_writers_educational" ]; then
        echo -e "${GRAY}Skipping educational version in quick mode${RESET}"
        return
    fi
    
    echo -e "${BOLD}${color}======================================================${RESET}"
    echo -e "${BOLD}${color}Testing ${desc} Implementation (${TIME_LIMIT} seconds)${RESET}"
    echo -e "${BOLD}${color}======================================================${RESET}"
    
    local exit_code=0
    
    if [ "$VERBOSE" = true ]; then
        READERS=$READERS WRITERS=$WRITERS OPERATIONS=$OPERATIONS timeout ${TIME_LIMIT}s ./$impl | grep -E "Total|Avg|Progress|\-\-\-" || true
        exit_code=${PIPESTATUS[0]}
    else
        READERS=$READERS WRITERS=$WRITERS OPERATIONS=$OPERATIONS timeout ${TIME_LIMIT}s ./$impl || true
        exit_code=$?
    fi
    
    # Display a message when timeout occurs
    if [ $exit_code -eq 124 ]; then
        echo -e "${YELLOW}Time limit reached (${TIME_LIMIT}s). Moving to next implementation...${RESET}"
    fi
    
    # If in benchmark mode, extract the statistics
    if [ "$BENCHMARK" = true ]; then
        # Run a tiny benchmark just to get stats reliably
        echo -e "${GRAY}Running quick benchmark for statistics collection...${RESET}"
        
        # Run with controlled small parameters
        STATS_OUTPUT=$(READERS=3 WRITERS=2 OPERATIONS=1 ./$impl 2>&1)
        
        # Extract values with safer defaults, accounting for different output formats
        TOTAL_READS=$(echo "$STATS_OUTPUT" | grep -E "Total reads|Completed reads" | tail -1 | grep -o -E '[0-9]+' | head -1 || echo "N/A")
        TOTAL_WRITES=$(echo "$STATS_OUTPUT" | grep -E "Total writes|Completed writes" | tail -1 | grep -o -E '[0-9]+' | head -1 || echo "N/A")
        
        # Handle different formatting of wait times
        READER_WAIT=$(echo "$STATS_OUTPUT" | grep -E "reader wait|Reader wait" | tail -1 | grep -o -E '[0-9]+(\.[0-9]+)?' | head -1 || echo "N/A")
        WRITER_WAIT=$(echo "$STATS_OUTPUT" | grep -E "writer wait|Writer wait" | tail -1 | grep -o -E '[0-9]+(\.[0-9]+)?' | head -1 || echo "N/A")
        
        # Add to results file
        echo "$desc,$READERS,$WRITERS,$OPERATIONS,$TOTAL_READS,$TOTAL_WRITES,$READER_WAIT,$WRITER_WAIT" >> "$RESULTS_FILE"
        
        # Print summary
        echo -e "${BOLD}${color}Summary for ${desc}:${RESET}"
        echo -e "  Total reads: ${BOLD}$TOTAL_READS${RESET}, Total writes: ${BOLD}$TOTAL_WRITES${RESET}"
        echo -e "  Avg reader wait: ${BOLD}$READER_WAIT ms${RESET}, Avg writer wait: ${BOLD}$WRITER_WAIT ms${RESET}"
        echo ""
    fi
    
    echo -e "${color}------------------------${RESET}"
}

# Run all implementations
echo -e "${YELLOW}Step 2: Running demonstration...${RESET}"

for i in "${!IMPLEMENTATIONS[@]}"; do
    run_implementation $i
done

# If benchmark mode, generate a simple report
if [ "$BENCHMARK" = true ]; then
    echo -e "${YELLOW}Step 3: Generating benchmark report...${RESET}"
    echo -e "${BOLD}Benchmark Results Summary:${RESET}"
    
    # Display the benchmark results in a table
    echo -e "${BOLD}Implementation   | Reads | Writes | Reader Wait | Writer Wait${RESET}"
    echo "----------------+-------+--------+-------------+-----------"
    
    if [ -f "$RESULTS_FILE" ]; then
        while IFS=, read -r impl readers writers ops reads writes reader_wait writer_wait; do
            # Skip header row
            if [ "$impl" != "Implementation" ]; then
                # Replace empty or N/A values with dashes
                reads=${reads:-"-"}
                writes=${writes:-"-"}
                reader_wait=${reader_wait:-"-"}
                writer_wait=${writer_wait:-"-"}
                
                # Format wait times with ms suffix if numeric
                if [[ "$reader_wait" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    reader_wait="${reader_wait}ms"
                fi
                if [[ "$writer_wait" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    writer_wait="${writer_wait}ms"
                fi
                
                printf "%-15s | %5s | %6s | %11s | %10s\n" "$impl" "$reads" "$writes" "$reader_wait" "$writer_wait"
            fi
        done < "$RESULTS_FILE"
        
        echo ""
        echo -e "${GREEN}Full results saved to: ${RESET}$RESULTS_FILE"
    else
        echo -e "${RED}Error: Results file not found${RESET}"
    fi
fi

echo -e "${BOLD}======================================================${RESET}"
echo -e "${GREEN}Demonstration completed successfully!${RESET}"
echo -e "${BOLD}======================================================${RESET}"
echo ""
echo "For more detailed information, see:"
echo "  - Report.md: Comprehensive project report with all implementation details"
echo "  - README.md: Installation and usage guide"
echo ""

exit 0
