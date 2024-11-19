#!/bin/python3
import re
import sys

def convert_trace_to_malloclab(input_file, output_file):
    allocations = {}
    allocation_id = 0
    total_bytes_allocated = 0
    total_operations = 0

    trace_contents = []

    with open(input_file, 'r') as f_in:
        for line in f_in:
            alloc_match = re.match(r'.*\+ (0x[0-9a-fA-F]+) (0x[0-9a-fA-F]+)', line)
            if alloc_match:
                ptr, size_hex = alloc_match.groups()
                size = int(size_hex, 16)

                allocations[ptr] = {
                    'id': allocation_id,
                    'size': size
                }

                trace_contents.append(f"a {allocation_id} {size}")

                total_bytes_allocated += size
                total_operations += 1

                allocation_id += 1
                continue

            free_match = re.match(r'.*- (0x[0-9a-fA-F]+)', line)
            if free_match:
                ptr = free_match.group(1)
                if ptr in allocations:
                    trace_contents.append(f"f {allocations[ptr]['id']}")
                    del allocations[ptr]
                    total_operations += 1
                continue

    with open(output_file, 'w') as f_out:
        f_out.write("1\n")
        f_out.write(f"{allocation_id}\n")
        f_out.write(f"{total_operations}\n")
        f_out.write(f"{total_bytes_allocated}\n")

        f_out.write("\n".join(trace_contents))

    print(f"Converted trace saved to {output_file}")
    print(f"Total unique allocations tracked: {allocation_id - 1}")

    if allocations:
        print("\nWarning: Unfreed allocations:")
        for ptr, info in allocations.items():
            print(f"ID {info['id']}: {ptr} (size {info['size']} bytes)")

if __name__ == "__main__":
    input_file = sys.argv[1] if len(sys.argv) > 1 else "input_trace.txt"
    output_file = sys.argv[2] if len(sys.argv) > 2 else "mm.trace"

    convert_trace_to_malloclab(input_file, output_file)
