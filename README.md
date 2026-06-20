# Matcha

An ultra-low latency, deterministic C++ Limit Order Book (LOB) matching engine built from scratch for predictable, sub-microsecond execution.

Designed to bypass the traditional overheads of high-level matching components by enforcing strict structural optimizations: a custom fixed-capacity memory arena that completely eliminates runtime heap allocations (`new`/`malloc`) on the hot-path, a flat array-mapped price ladder to prevent tree rebalancing penalties, and an in-place zero-allocation parser.

---

## Core Concepts

### Zero-Allocation Memory Arena

Traditional dynamic memory allocation introduces unpredictable latency jitter via heap contention, thread synchronization, and potential OS context switches. Matcha instantiates its entire memory footprint at startup:

* **Storage Array:** Active and recycled orders live inside a single, contiguous block of memory (`mArena`).
* **Singly-Linked Free List:** An internal index array (`mFreeList`) tracks unallocated slots. Allocations and deallocations run in deterministic $O(1)$ time by serving or returning indexes from the head of the free list pool, keeping data tightly localized.

### Flat Array Price Ladder

To bypass the pointer-chasing and rebalancing overheads of tree structures (like `std::map`) or the bucket collision risks of hash tables (`std::unordered_map`), the engine bounds the trading space to a dedicated price horizon:

* **Direct-Offset Mapping:** Price levels map directly to slots inside a contiguous array. Resolving a price level reduces to simple pointer arithmetic:

$$\text{Index} = \text{Target Price} - \text{Base Price} + \text{HALF\_WIDTH}$$


* **Doubly-Linked FIFO Queues:** Each active `PriceLevel` maintains indices pointing to its `oldestOrderID` and `newestOrderID`. Orders at the same price tier form an intrusive doubly-linked list woven directly inside the memory arena, guaranteeing true time-priority execution loops without auxiliary container costs.

### Non-Allocating String Tokenizer

The ingress pipeline avoids string slicing, copying, or token allocation when consuming CSV streams. The parser walks a raw `std::string_view` byte-by-byte, resolving numbers in-place via primitive accumulation and signaling branches directly to the compiler using predictability hints (`__builtin_expect`).

---

## Architecture Overview

```
                        [ Inbound CSV Line ]
                                 │
                                 ▼
                         Parser::parse()
                 (In-place std::string_view scan)
                                 │
                                 ▼
                     OrderBook::consumeOrder()
                                 │
         ┌───────────────────────┴───────────────────────┐
         ▼                                               ▼
   [ BUY Order ]                                   [ ASK Order ]
         │                                               │
         ▼                                               ▼
Match against mAsks.bestPrice()                 Match against mBuys.bestPrice()
(Price-Improvement Rule:                        (Price-Improvement Rule:
 Trades execute at resting maker price)          Trades execute at resting maker price)
         │                                               │
         ├─── Fully Filled? ───► [ Free Memory Index ]   ├─── Fully Filled? ───► [ Free Memory Index ]
         │                                               │
         └─── Remaining Volume ─┐                        └─── Remaining Volume ─┐
                                ▼                                               ▼
                    mBuys.emplaceOrder()                            mAsks.emplaceOrder()
                (Intrusive FIFO Tail Append)                    (Intrusive FIFO Tail Append)

```

---

## Technical Specifications

### Data Layout Constraints

To maximize cache-line efficiency and prevent compiler padding overheads, the core data entities are packed flat:

```cpp
struct Order {
  uint32_t orderID;
  uint32_t orderPrice;
  uint32_t orderQty;
  uint32_t nextOrder;
  uint32_t prevOrder;
  char orderType;
} __attribute__((packed));

```

### Supported API Transactions

* **Add (`A`)**: Allocates an order index, scans the opposing side's cached `mBestPrice` to cross crossing orders, updates execution states, and anchors remaining volume onto the price ladder.
* **Cancel (`C`)**: Decouples an order from its specific price level queue in $O(1)$ time by splicing its intrusive forward and backward pointer links inside the arena.
* **Replace (`R`)**: Modifies order parameter state securely. If the transaction changes the price level or expands volume, priority is systematically stripped and repositioned at the tail of the respective queue.

---

## Execution Flow

```
main()
  Initialize OrderBook with a contract BASE_PRICE
  Read lines from std::cin → stream into stack-allocated buffer

Parser::parse()
  Scan characters via data pointer walking
  Convert ASCII digits directly to integers 
  Populate ParsedOutput struct without heap string splitting

OrderBook::consumeOrder()
  Intercept command flag (A, R, C)
  If Cross Event Detected:
    Walk active PriceLevel queues using oldestOrderID -> nextOrder links
    Generate TradeEvents evaluating match quantity
    Enforce Price-Improvement Rule: settle trades at the resting maker's limit price
    Evict depleted nodes directly to the MemManager free list
  If Residual Volume Left:
    Fetch free block from MemManager
    Append node to the tail of the corresponding PriceLadder chain

```

---

## Compilation and Execution

### Build Instructions

Matcha requires a C++17 compliant compiler (`gcc` or `clang`) and CMake >= 3.15.

```bash
mkdir build bin
chmod +x scripts/*
./scripts/build

```

### Running the Engine

The engine reads transactions continuously from standard input (`std::cin`) using a structured comma-separated format:

```bash
bin/matcha < test_suite.csv

```

### Input Format Reference

```text
A,1,B,10050,10  (Action: Add, OrderID: 1, Side: Bid, Price: 10050, Qty: 10)
A,2,A,10050,10  (Action: Add, OrderID: 2, Side: Ask, Price: 10050, Qty: 10)
C,1,B           (Action: Cancel, OrderID: 1, Side: Bid)
R,2,A,10055,15  (Action: Replace, OrderID: 2, Side: Ask, New Price: 10055, New Qty: 15)

```

---

## Limitations

* **Single-Threaded Context:** Optimized strictly for sequential execution on an isolated CPU core; concurrent cross-thread race conditions are not guarded.
* **Bounded Price Horizon:** Flat price ladder lookups rely on a rigid range limit (`HALF_WIDTH = 10000`) derived from the contract's baseline reference value. Orders extending past this threshold breach safety invariants. Adjust base-price to be close to the median of your input price data.
* **External Tracking Maps:** Order IDs map to underlying arena offsets using `std::unordered_map`. While lookup is fast, bucket chaining can introduce minor structural latency variances on cancellation calls compared to pure dense pointer tracking.

## Next Steps

* **ITCH 5.0 Binary Protocol Parsing:** The current engine accepts custom CSV-data as input, but teh next stage in this project is to scrap that for a binary format parser in order to benchmark against historical data.
