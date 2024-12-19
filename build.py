#!/usr/bin/python3

import subprocess
import sys

from itertools import product

def build_program(best_fit_search_limit,
                  mini_block_optimization,
                  free_list_insert_strategy,
                  free_table_size,
                  binning_strategy,
                  run_name):

    if mini_block_optimization:
        mini_block_optimization = "TRUE"
    else:
        mini_block_optimization = "FALSE"

    CC = "gcc"
    FLAGS = "-std=gnu11 -lm"
    FLAGS += " -Wall -Wextra -Wpedantic -Werror"
    FLAGS += " -Wdouble-promotion -Wno-type-limits -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function"

    # integer
    FLAGS += f" -DBEST_FIT_SEARCH_LIMIT={best_fit_search_limit}"

    # TRUE, FALSE
    FLAGS += f" -DMINI_BLOCK_OPTIMIZATION={mini_block_optimization}"

    # ADDRESS_ORDERED, FILO
    FLAGS += f" -DFREE_LIST_INSERT_STRATEGY={free_list_insert_strategy}"

    # integet
    FLAGS += f" -DFREE_TABLE_SIZE={free_table_size}"

    # Linear_Binning, Exponential_Binning, Hybrid_Binning, Range_Binning
    FLAGS += f" -DSize_Get_Bin_Index={binning_strategy}"

    FLAGS += f' -DRUN_NAME="{run_name}"'

    RELEASE_FLAGS = "-O3 -DNDEBUG"

    SRC = [
        "main.c",
        "trace.c",
        "trace_parser.c",
        "mm.c",
        "heapsim.c",
        "perf.c",
        "vec_u64.c",
        "string.c",
        "csv.c",
    ]

    build_flags = f"{FLAGS} {RELEASE_FLAGS}"

    command = [CC] + build_flags.split() + SRC + ["-o", "main"]

    try:
        print(command);
        subprocess.run(command, check=True)
    except subprocess.CalledProcessError as e:
        print("build failed...")
        sys.exit(1)

    try:
        print("./main");
        subprocess.run("./main", check=True)
    except subprocess.CalledProcessError as e:
        print("run failed...")
        sys.exit(1)


# --- effect of binning strategies...

# for b in ["Linear_Binning", "Exponential_Binning", "Hybrid_Binning", "Range_Binning"]:
#     build_program(0x10, True, "FILO", 0x10, b, b)

# --- mini block optimization...

# build_program(0x10, True, "FILO", 0x1, "Linear_Binning", "Mini_Block_No_Binning")
# build_program(0x10, True, "FILO", 0x10, "Linear_Binning", "Mini_Block_With_Binning")

# build_program(0x10, False, "FILO", 0x1, "Linear_Binning", "No_Mini_Block_No_Binning")
# build_program(0x10, False, "FILO", 0x10, "Linear_Binning", "No_Mini_Block_With_Binning")

# --- search strategies...

# for search_limit in [0x1, 0x8, 0x10, 0x18, 0x20, 0x28, 0x30]:
#     build_program(search_limit, True, "FILO", 0x10, "Linear_Binning", f"Search_Limit_{search_limit}")

# --- LL ordering...
# build_program(0x10, True, "FILO", 0x10, "Linear_Binning", "FILO_Insertion")
# build_program(0x10, True, "ADDRESS_ORDERED", 0x10, "Linear_Binning", "Address_Ordered_Insertion_1")
# build_program(0x20, True, "ADDRESS_ORDERED", 0x10, "Linear_Binning", "Address_Ordered_Insertion_2")
# build_program(0x30, True, "ADDRESS_ORDERED", 0x10, "Linear_Binning", "Address_Ordered_Insertion_3")
# build_program(0x40, True, "ADDRESS_ORDERED", 0x10, "Linear_Binning", "Address_Ordered_Insertion_4")
# build_program(0x50, True, "ADDRESS_ORDERED", 0x10, "Linear_Binning", "Address_Ordered_Insertion_5")
