import sys
import os
import subprocess


def test_file(file):
    print(f"Testing file: {file}")

    process = subprocess.Popen(
        ["build/lama-insnfreq-analysis", "--input", file, "--threshold", "1"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    actual_output, stderr = process.communicate()

    if process.returncode != 0:
        print(f"Error {process.returncode}!")
        print("Stderr:")
        print(stderr)
        print("Stdout:")
        print(actual_output)
        return True

    return False


test_files = [
    dir + "/" + f
    for dir in [
        "../performance",
        "../regression",
        "../regression_long/expressions",
        "../regression_long/deep-expressions",
    ]
    for f in os.listdir(dir)
    if f.endswith(".bc")
]

for filename in sorted(test_files):
    if test_file(filename):
        sys.exit(1)
