#!/bin/python3

import sys

def parse_mtrace_to_malloclab(input_file, output_file):
    allocation_map = {}  # Maps memory addresses to IDs
    next_id = 0          # Unique ID counter
    malloclab_logs = []

    num_ids = 0          # Total number of unique IDs (allocations)
    num_ops = 0          # Total number of operations
    max_alloc = 0        # Maximum bytes allocated at any point
    current_alloc = 0    # Current allocated bytes

    def allocate(address, size):
        nonlocal next_id, num_ids, current_alloc, max_alloc, num_ops
        num_ops += 1
        allocation_map[address] = next_id
        malloclab_logs.append(f"a {next_id} {int(size, 16)}")  # Convert size to decimal
        next_id += 1
        num_ids += 1
        current_alloc += int(size, 16)
        max_alloc = max(max_alloc, current_alloc)

    def free(address):
        nonlocal current_alloc, num_ops
        if address in allocation_map:
            num_ops += 1
            malloclab_logs.append(f"f {allocation_map[address]}")
            del allocation_map[address]

    def realloc(old_address, new_address, size):
        nonlocal current_alloc, max_alloc, num_ops
        if old_address in allocation_map:
            num_ops += 1
            malloclab_logs.append(f"r {allocation_map[old_address]} {int(size, 16)}")  # Convert size to decimal
            new_size = int(size, 16)
            old_size = allocation_map.get(old_address, 0)
            current_alloc += (new_size - old_size)
            max_alloc = max(max_alloc, current_alloc)
            if new_address != old_address:
                allocation_map[new_address] = allocation_map.pop(old_address)

    with open(input_file, 'r') as infile:
        for line in infile:
            line = line.strip()
            if line.startswith('@'):
                parts = line.split()
                if len(parts) < 4:
                    continue
                operation = parts[2]
                address = parts[3]

                if operation == "+":
                    # Allocation
                    size = parts[4]
                    allocate(address, size)
                elif operation == "-":
                    # Free
                    free(address)
                elif operation == ">":
                    # Realloc
                    size = parts[4]
                    new_address = parts[3]
                    realloc(address, new_address, size)

    with open(output_file, 'w') as outfile:
        # Write header based on spec
        outfile.write(f"0\n")  # Weight
        outfile.write(f"{num_ids}\n")  # Total IDs
        outfile.write(f"{num_ops}\n")  # Total operations
        outfile.write(f"{max_alloc}\n")  # Maximum allocated bytes
        for log in malloclab_logs:
            outfile.write(log + "\n")

    print(f"Parsed malloclab logs saved to {output_file}")


input_mtrace_log = sys.argv[1]
output_malloclab_log = sys.argv[2]
parse_mtrace_to_malloclab(input_mtrace_log, output_malloclab_log)

