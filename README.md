# CloudSuite Memcached Loader

This repository contains a CloudSuite Memcached benchmarking loader with support for multiple workload modes:

- default CloudSuite/Twitter skewed access pattern
- sequential access over the loaded Twitter/key-value distribution
- synthetic fixed-size workload generation with deterministic fixed-format keys
- optional GET-miss fill behavior, where a missed GET can enqueue a SET for the same key
- periodic CSV stats logging

The source builds a single executable named `loader`.

## Repository Layout

```text
.
  loader.c              Command-line parsing, setup, stats CSV output
  generate.c/.h         Key generation, request generation, distributions
  worker.c/.h           Worker threads, event loop, pacing, warmup
  request.c/.h          Memcached binary protocol request creation/sending
  response.c/.h         Response parsing, hit/miss accounting, miss fill
  stats.c/.h            Periodic stats aggregation and CSV printing
  conn.c/.h             TCP connection setup
  tenants.txt           Memcached tenant/target list
  Makefile              Build recipe

../twitter_dataset/
  twitter_dataset_unscaled
  twitter_dataset_2     Optional, if present in your checkout
```

## Build

### Linux

Install build tools and libevent development headers, then run:

```bash
cd src
make clean
make
```

Typical packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential libevent-dev
```

### macOS

The Makefile can find Homebrew libevent automatically:

```bash
brew install libevent
make clean
make
```

## Tenant File Format

`tenants.txt` lists the memcached targets:

```text
hostname_or_ip, port, share
```

Example:

```text
localhost, 11212, 1
```

The `share` value controls each server's share of the generated request rate. If you run multiple servers, the number of worker threads must be divisible by the number of servers.

## Workload Modes

### 1. Default Twitter Skewed Workload

This is the normal CloudSuite behavior. The loader reads the Twitter/key-value distribution from `-a`, loads it into memory, and samples keys using the CDF from that dataset.

Preload:

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_2 \
  -s tenants.txt \
  -w 2 \
  -S 1 \
  -D 2048 \
  -j \
  -T 1 \
  -r 100000 \
  -C ./preload_stats.csv
```

Main phase:

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_2 \
  -s tenants.txt \
  -g 1 \
  -T 1 \
  -c 25 \
  -w 1 \
  -r 25000 \
  -C ./run_stats.csv
```

### 2. Sequential Dataset Workload

Use `-q` to walk sequentially through the loaded Twitter/key-value distribution instead of sampling from the skewed CDF. Each worker starts at a slightly different position and wraps around at the beginning of the distribution.

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_2 \
  -s tenants.txt \
  -q \
  -g 1 \
  -T 1 \
  -c 25 \
  -w 1 \
  -r 25000 \
  -C ./sequential_stats.csv
```

To also fill GET misses with SET requests, add `-R`:

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_2 \
  -s tenants.txt \
  -q \
  -R \
  -g 1 \
  -T 1 \
  -c 25 \
  -w 1 \
  -r 25000 \
  -C ./sequential_fill_stats.csv
```

### 3. Synthetic Fixed-Size Workload

Use synthetic mode to ignore the Twitter CDF and generate a fixed keyspace:

- `-y` enables synthetic mode explicitly.
- `-b <seed>` sets the deterministic base seed and also enables synthetic mode.
- `-k <count>` sets the number of generated keys.
- `-f <bytes>` sets the fixed object size.

Synthetic keys are generated as deterministic strings:

```text
k0000000000000000
k0000000000000001
k0000000000000002
...
```

Synthetic preload:

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_unscaled \
  -s tenants.txt \
  -w 1 \
  -S 1 \
  -D 2048 \
  -j \
  -T 1 \
  -r 20000 \
  -y \
  -f 650 \
  -k 1230000 \
  -b 12345 \
  -C ./synthetic_preload_stats.csv
```

Synthetic main phase:

```bash
./loader \
  -a ../twitter_dataset/twitter_dataset_unscaled \
  -s tenants.txt \
  -g 1 \
  -T 1 \
  -c 25 \
  -w 1 \
  -r 20000 \
  -y \
  -f 650 \
  -k 1230000 \
  -b 12345 \
  -C ./synthetic_run_stats.csv
```

The `-a` argument is still required by the loader interface, but synthetic mode does not load the dependency distribution.

## Important Flags

| Flag | Meaning |
| --- | --- |
| `-a <file>` | Input Twitter/key-value distribution file. Required. |
| `-s <file>` | Tenant/target file. Required. |
| `-C <file>` | Stats CSV output path. If omitted, the loader uses the legacy `/users/AMH/cloudsuite_<client_ip>_<port>.csv` path. |
| `-j` | Preload keys with SET requests, then exit after warmup completes. |
| `-g <fraction>` | Fraction of generated operations that are GETs. `-g 1` means all GETs. |
| `-r <rps>` | Attempted aggregate requests per second. `0` or omitted means run as fast as possible. |
| `-T <seconds>` | Stats print interval. |
| `-t <seconds>` | Runtime limit. Omit to run forever. |
| `-w <count>` | Worker threads. Must be divisible by the number of servers. |
| `-c <count>` | Total connections across all workers. |
| `-D <MB>` | Memcached memory per server, used to estimate preload key count for dataset mode. |
| `-S <factor>` | Dataset scaling factor. Scaling above 1 requires `-o`. |
| `-o <file>` | Output distribution file when scaling the input dataset. |
| `-f <bytes>` | Fixed object size. Required for synthetic mode. |
| `-k <count>` | Number of synthetic keys, or uniform keyspace size when no popularity file is used. |
| `-b <seed>` | Base RNG seed. Also enables synthetic mode. |
| `-y` | Enable synthetic mode explicitly. |
| `-q` | Enable sequential access over the loaded dataset. |
| `-R` | On GET miss, enqueue a SET for the missed key. |
| `-N <file>` | Key popularity distribution file with 10,000 CDF rows. |
| `-d <file>` | Value-size distribution file with 10,000 CDF rows. |
| `-e` | Use exponential interarrival distribution. Default is constant pacing. |
| `-m <fraction>` | Fraction of GETs that are multiget. |
| `-l <count>` | Fixed number of keys per multiget. |
| `-L <file>` | Multiget-size distribution file. |
| `-i <fraction>` | Fraction of operations that are INCR. |
| `-n` | Enable Nagle's algorithm. |
| `-u` | Use UDP mode. TCP is the default. |
| `-x` | Run timing tests and exit. |

## Dataset Formats

### Twitter/key-value input file

Each row should contain:

```text
cdf, value_size, key
```

The loader reads the file, stores each key and value size, and inserts key-to-size mappings into an internal hash table. Dataset mode uses this distribution for skewed sampling and sequential mode uses the same loaded entries in order.

### Distribution files

Files passed through `-N`, `-d`, or `-L` must contain exactly 10,000 rows. The loader reads the second column from each row as the sampled value.

## Stats CSV

The first row contains:

```text
ts,timeDiff,rps,requests,gets,sets,hits,misses,avg_lat,90th,95th,99th,std,min,max,avgGetSize
```

Latency values are printed in milliseconds. `ts` is wall-clock epoch time with nanosecond precision.

Use `-C` to choose a convenient location:

```bash
./loader ... -C ./results/cloudsuite_run.csv
```

Make sure the output directory already exists.

## Preload Behavior

With `-j`, the loader sends SET requests and exits after warmup completes.

Dataset mode estimates `keysToPreload` from:

```text
server_memory_MB / average_object_size
```

Synthetic mode sets `keysToPreload` to the synthetic key count from `-k`.

## GET-Miss Fill Behavior

`-R` enables an additional behavior that is not part of stock CloudSuite: when a GET or GETQ returns `KEY_NOT_FOUND`, the worker creates and sends a SET for the same key.

Value size selection for the fill SET:

1. use `-f` if a fixed size is configured
2. otherwise use the loaded dataset's key-to-size mapping when available
3. otherwise sample the value-size distribution

This is useful for experiments where misses should gradually populate the cache during the run.

## Reproducibility

Synthetic mode is deterministic when `-b` is fixed. Each worker receives a deterministic per-worker seed derived from the base seed and worker id.

Example:

```bash
-b 12345 -f 650 -k 1230000
```

Using the same binary, seed, key count, worker count, and request-rate settings will reproduce the same synthetic key selection behavior.

## Troubleshooting

### `event2/event.h` not found

Install libevent development headers:

```bash
sudo apt-get install -y libevent-dev
```

or on macOS:

```bash
brew install libevent
```

### `Could not open stats CSV file`

The directory passed to `-C` does not exist or is not writable. Create the directory first or choose a path in the current directory.

### `Number of client (worker) threads must be divisible by the number of servers`

If `tenants.txt` lists two servers, use `-w 2`, `-w 4`, and so on.

### Synthetic mode requires `-f`

Synthetic mode needs a fixed object size:

```bash
-y -f 650 -k 1230000
```

Using `-b` also enables synthetic mode, so the same requirement applies.

### Port mismatch

Edit `tenants.txt` to match your running memcached host and port.

## Typical Experiment Flow

1. Start memcached with the memory size and port you want to test.
2. Edit `tenants.txt` to point at the memcached server.
3. Build the loader with `make`.
4. If you need a larger scaled dataset, run preload with `-j`, `-S <factor>`, and `-o <scaled_output_file>`.
5. Otherwise, preload with `-j` and `-S 1`.
6. Run the main phase without `-j`. Use `-g` to choose the GET fraction; values below `1` also generate SET requests.
7. Save each run with a unique `-C` CSV output path.

Example:

```bash
make clean && make

./loader -a ../twitter_dataset/twitter_dataset_2 -s tenants.txt -w 1 -j -D 2048 -S 4 -o ./twitter_dataset_scaled_x4 -r 100000 -T 1 -C ./preload_scaled.csv

./loader -a ./twitter_dataset_scaled_x4 -s tenants.txt -w 1 -c 25 -g 0.95 -r 25000 -T 1 -C ./main_phase.csv
```
