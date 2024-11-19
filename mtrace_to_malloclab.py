#!/bin/python3
import re
import sys

def convert_trace_to_malloclab(input_file, output_file):
    """
    Convert a raw memory trace to malloc lab style trace format with allocation IDs
    Trace format:
    - 'a id size': Allocate memory with unique ID and size
    - 'f id': Free memory using allocation ID
    """
    # Dictionary to track allocations by pointer and ID
    allocations = {}
    allocation_id = 0
    total_bytes_allocated = 0
    total_operations = 0

    # Temporary storage for trace contents
    trace_contents = []

    with open(input_file, 'r') as f_in:
        for line in f_in:
            # Match allocation lines (+ symbol)
            alloc_match = re.match(r'.*\+ (0x[0-9a-fA-F]+) (0x[0-9a-fA-F]+)', line)
            if alloc_match:
                ptr, size_hex = alloc_match.groups()
                # Convert hex size to decimal
                size = int(size_hex, 16)

                # Store allocation with unique ID
                allocations[ptr] = {
                    'id': allocation_id,
                    'size': size
                }

                # Write allocation with ID
                trace_contents.append(f"a {allocation_id} {size}")

                # Track total bytes and operations
                total_bytes_allocated += size
                total_operations += 1

                # Increment allocation ID
                allocation_id += 1
                continue

            # Match free lines (- symbol)
            free_match = re.match(r'.*- (0x[0-9a-fA-F]+)', line)
            if free_match:
                ptr = free_match.group(1)
                if ptr in allocations:
                    # Write free command with only allocation ID
                    trace_contents.append(f"f {allocations[ptr]['id']}")
                    del allocations[ptr]
                    total_operations += 1
                continue

    # Prepare to write trace file
    with open(output_file, 'w') as f_out:
        # Write 4-line header according to spec
        f_out.write("1\n")  # Default weight
        f_out.write(f"{allocation_id}\n")  # Number of allocation IDs
        f_out.write(f"{total_operations}\n")  # Total number of operations
        f_out.write(f"{total_bytes_allocated}\n")  # Maximum bytes allocated

        # Write trace contents
        f_out.write("\n".join(trace_contents))

    print(f"Converted trace saved to {output_file}")
    print(f"Total unique allocations tracked: {allocation_id - 1}")

    # Optional: Print any unfreed allocations
    if allocations:
        print("\nWarning: Unfreed allocations:")
        for ptr, info in allocations.items():
            print(f"ID {info['id']}: {ptr} (size {info['size']} bytes)")

# Example usage
if __name__ == "__main__":
    input_file = sys.argv[1] if len(sys.argv) > 1 else "input_trace.txt"
    output_file = sys.argv[2] if len(sys.argv) > 2 else "mm.trace"

    convert_trace_to_malloclab(input_file, output_file)
