import os, subprocess, sys
from helpers.helpers import *
from assemble import assemble_all_files
from typing import List
import time
import csv
from pathlib import Path

def compile_file(program_path, compiler_args: List[str]):
    program = os.path.basename(program_path)

    root_dir = resolve_path(os.path.dirname(__file__), "../../../")
    integration_dir = os.path.dirname(__file__)
    stdlib_dir = os.path.join(integration_dir, "../../stdlib")
    stdlib_files = get_all_files(stdlib_dir, ".java")
    joosc_executable = resolve_path(root_dir, "./joosc")
    files = get_all_files(program_path, ".java") if os.path.isdir(program_path) else [program_path]

    result = subprocess.run([joosc_executable, *compiler_args, *files, *stdlib_files])

    if result.returncode in (0, 43):
        assembled = assemble_all_files()
        if not assembled:
            print(f"{colors.FAIL}FAIL: joosc failed to assemble {program}, so it couldn't be run.{colors.ENDC}\n")
            return False

def add_to_csv(optimization:str, benchmark_name: str, optimized_time: float, unoptimized_time: float):
    root_dir = resolve_path(os.path.dirname(__file__), "../../../")
    results_file = os.path.join(root_dir, "benchmarks/results.csv")

    if not os.path.exists(results_file):
        file = open(results_file, "w", newline='')
        writer = csv.writer(file)
        header_field = ["optimization", "benchmark_name", "time in ms w/ opt", "time in ms w/o opt", "speedup"]
        writer.writerow(header_field)
        file.close()
    
    speedup = unoptimized_time / optimized_time
    new_row = [optimization, benchmark_name, str(optimized_time), str(unoptimized_time), str(speedup)]

    with open(results_file, "a", newline='') as file:
        writer = csv.writer(file)
        writer.writerow(new_row)
        file.close()


def run_benchmark(optimization: str):
    root_dir = resolve_path(os.path.dirname(__file__), "../../../")
    benchmarks_dir = os.path.join(root_dir, "benchmarks")
    optimization_dir = os.path.join(benchmarks_dir, optimization)

    # empty results file
    results_file = os.path.join(root_dir, "benchmarks/results.csv")
    os.remove(results_file)

    for file in os.listdir(optimization_dir):
        program_path = os.path.join(optimization_dir, file)
        print(f"{colors.OKGREEN}Compiling {file} without optimization{colors.ENDC}")
        compile_file(program_path, ['--opt-none'])

        print(f"Running unoptimized program")
        total_ms_unoptimized = 0
        for i in range(100):
            start = time.time()
            subprocess.run(["./main"], cwd=root_dir)
            end = time.time()
            total_ms_unoptimized += (end - start) * 1000

        avg_ms_unoptimized = total_ms_unoptimized / 100
        print(f"Finished running unoptimized program, runtime: {colors.BOLD}{avg_ms_unoptimized} ms{colors.ENDC}")

        print(f"{colors.OKGREEN}Compiling {file} with {optimization} optimization{colors.ENDC}")
        compile_file(program_path, ["--optimized", optimization])

        print(f"Running optimized program")
        total_ms_optimized = 0
        for i in range(100):
            start = time.time()
            subprocess.run(["./main"], cwd=root_dir)
            end = time.time()
            total_ms_optimized += (end - start) * 1000

        avg_ms_optimized = total_ms_optimized / 100
        print(f"Finished running optimized program, runtime: {colors.BOLD}{avg_ms_optimized} ms{colors.ENDC}")

        add_to_csv(optimization, file, avg_ms_optimized, avg_ms_unoptimized)

def get_benchmarks():
    root_dir = resolve_path(os.path.dirname(__file__), "../../../")
    benchmarks_dir = os.path.join(root_dir, "benchmarks")

    return [f for f in os.listdir(benchmarks_dir) if f != "results.csv"]


if __name__ == "__main__":


    load_env_file()
    benchmarks = get_benchmarks()
    for benchmark in benchmarks:
        run_benchmark(benchmark)

