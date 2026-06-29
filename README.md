# Matcha

A deterministic C++ limit order book matching engine built for low-latency execution. Matcha parses NASDAQ ITCH 5.0 binary data, maintains a price-time priority order book, and logs all matched trades.

---

## How It Works

### Memory Arena

Matcha allocates its entire memory footprint at startup — no heap allocations occur on the hot path. Orders live inside a fixed-capacity contiguous array, with a free list tracking available slots. Allocation and deallocation are both O(1).

### Price Ladder

Price levels map directly to slots in a flat array via tick-aligned arithmetic:

```
slot = (price - base_price) / tick_size + HALF_WIDTH
```

Each price level maintains a doubly-linked FIFO queue of orders woven directly through the arena, giving true price-time priority without auxiliary containers.

### ITCH 5.0 Parser

The parser mmaps the input binary file and walks it using a lightweight `BufReader`. Each MoldUDP64 packet is read, and ITCH messages are dispatched by type byte into typed structs via `memcpy`. All multi-byte fields are byte-swapped from network order on ingestion.

### Matching

Incoming orders are matched against the opposing side before being placed on the book. Trades execute at the resting maker's price. Residual volume after matching rests on the ladder at its limit price.

---

## Supported Message Types

| Type | Description |
|------|-------------|
| `A`  | Add Order |
| `F`  | Add Order with MPID |
| `X`  | Order Cancel (partial) |
| `D`  | Order Delete |
| `U`  | Order Replace |
| `S`  | System Event |

All other ITCH message types are silently ignored as they have no impact on the order book.

---

## Build and Run

Requires a C++17 compiler and CMake >= 3.15.

```bash
mkdir build bin
chmod +x scripts/*
./scripts/build
```

Run the engine against a binary input file:

```bash
bin/matcha <path_to_binary_file>
```

A tool for converting the supported message types from CSV format to the proper message binary format is also provided.

```bash
cd tools/itch-gen
../../scripts/build
cd ../.. 
bin/tools/itch-gen < path-to-your-csv > path-to-output-binary
```
The format for the input CSV file is to be as follows:
```csv
msgType, stockLocate, trackNum, timestamp, orderRefNum (old for replace orders), new orderRefNum (only for replace), buy-sell/event-code, shares, price, stock, attr
```

Each line in the CSV must have 11 fields/10 commas, and null fields must remain blank (whitespace is not treated as blank).
Also, while making the binary file using **itch-gen**, you will be prompted to enter the number of messages in your CSV file. If you wish to skip entering the number by yourself, then keep that number as the first line of your CSV file and enter -1 in the prompt.

You will be prompted to enter a base price. Set this as close to the median price of your input data as possible — the price ladder is bounded to `HALF_WIDTH = 10000` ticks on either side of the base.

Output is written to `trade.log` and `error.log` in the working directory.

---

## Sample Data

Two sample files are provided in `examples/`:

- `input.csv` — well-formed ITCH data that produces no trades (all orders rest on the book without crossing).
- `test.csv` — data designed to produce trades, useful for verifying matching logic end to end.

Pass either through your ITCH binary generator before feeding to the engine.

---

## Limitations

- **Single stock:** One order book per engine instance. Multi-stock routing is not implemented.
- **Single packet:** The parser currently handles a single MoldUDP64 packet per file. Multi-packet support is not yet implemented.
- **Bounded price horizon:** Orders outside the ladder range are rejected. Keep your base price close to the median of your input data.
- **No authentication or session management:** MoldUDP64 session fields are parsed but ignored.

---

## Next Steps

- **Generation-based arena allocator:** Replace the current bump allocator and free list with a slot generation counter to detect use-after-free and double-free at O(1) cost.
