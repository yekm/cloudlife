# CPU concurrency utilities

The header-only C++17 utilities in `src/utils/concurrency/` have no dependency on
Art, ImGui, Easel, or OpenGL. Workers compute CPU data; the render thread consumes
results and performs all drawing and GPU uploads.

## Thread ownership

`WorkerThreads` owns persistent threads running a supplied worker loop. Its stop
callback sets the loop's shutdown predicate and wakes every condition variable on
which a worker can wait. Shutdown then joins every thread. The same cleanup runs
if construction fails after starting some threads.

The stop callback must not throw, and exceptions must not escape the worker loop.
Call `shutdown()` from the owner, never from one of its workers. Place the thread
owner after all members referenced by its callbacks, and stop it before those
members are destroyed. Construction callbacks may run immediately; initialize
all their dependencies before constructing the thread owner.

## Blocking batches

`ParallelBatch(count)` starts persistent workers. Each `run(callback)` invokes
`callback(worker_index, worker_count)` once on every worker and returns only after
all callbacks finish. It rethrows the first callback exception after completing
the barrier; the workers remain available for the next batch.

Use one submitting thread, with no overlapping or recursive calls to `run()`.
Destruction must not overlap a batch. Partition output so callbacks write disjoint
elements, and read or resize that output only after `run()` returns. Marbling uses
interleaved rows and advances its animation state after this barrier.

```cpp
concurrency::ParallelBatch batch(4);
batch.run([&](size_t worker, size_t count) {
    for (size_t row = worker; row < height; row += count)
        compute_row(row);
});
```

## Asynchronous indexed work

`IndexedWorkerPool<Job, Result, WorkerState, Policy>` distributes indexed spans
across persistent workers. Each worker constructs its own `WorkerState`; a new
job initializes that state before computing its first span. `Job` derives from
`JobControl`, and its computation parameters are immutable after submission.

The policy supplies these static functions:

| Function | Purpose |
| --- | --- |
| `total(const Job&)` | Number of indexed elements |
| `eligible(const Job&, size_t worker)` | Whether this worker may claim work |
| `span_size(const Job&, size_t first)` | Positive span length within the remaining elements |
| `capacity(const Job&)` | Positive completed-result queue limit |
| `initialize(const Job&, WorkerState&)` | Reset worker-local state for a new job |
| `compute(Job&, WorkerState&, size_t first, size_t count, Result&)` | Produce an owned result; return false for an incomplete span |

The first four functions run under the scheduler mutex and must be cheap and
nonthrowing. Initialization and computation run outside that mutex. Computation
must check cancellation cooperatively and avoid shared mutable state. Results
may arrive out of order. Queue capacity bounds queued results; each active worker
can also hold one result while waiting to publish.

The owner submits a fresh `shared_ptr<Job>` through `start()`. Replacing a job
cancels the previous one, clears queued results, and rejects its late publications.
`start(nullptr)` cancels without a replacement; `set_paused()` controls the current
job. Use these pool methods rather than changing a submitted job's controls
directly, because they also wake scheduler and publisher waits. One owner thread
submits jobs and consumes results; destruction must not overlap owner operations.

`take(result)` transfers one completed result without waiting. `take_error()`
transfers a worker's exception for reporting on the owner thread. Initialization
or computation failures cancel their job and leave the pool reusable. Failures
constructing worker state or allocating queue storage stop the pool; subsequent
submissions throw, so replace the pool if recovering from those failures.

XLyap's policy owns row-aligned tile selection, right-edge exponent reuse, and
worker-local orbit initialization. The result queue stays inside the pool so job
replacement and stale-result rejection share one mutex.

## Job control

`JobControl` provides cooperative cancellation and pause/resume. Long computations
call `checkpoint()` at suitable boundaries; it returns false after cancellation
and waits while paused. Cancellation wakes paused workers.

Use `set_paused()` and `cancel()` once workers can access the control. Initial
atomic values may be assigned before starting a job. Reusing a control is safe
only after all workers using the previous job have finished. Cancellation cannot
interrupt an external library call that does not inspect the control.

## Snapshot publication

`LatestSnapshot<T>` holds at most one generation-tagged snapshot. Publishing
replaces an unconsumed snapshot; `take()` transfers ownership to the consumer.
Generation numbers must increase across requests. Older publications are rejected
even after the mailbox has been consumed. Multiple snapshots of one generation
are allowed; their publication order determines which one is retained.

The consumer must still compare the snapshot generation with its latest request:
a cancelled job can finish publishing before its replacement has published any
data. Prime UMAP uses this mailbox for copied, packed vertex data. Its live UMAP
embedding stays private to the background computation.

## Art-specific computation

Keep formulas, work partitioning, random-number state, caches, palette application,
and GPU operations in each art. Shared scheduling does not make sequential
algorithms parallel or make mutable UI settings safe to read from workers.

Marbling continues to wait for a complete frame before reading the shared pixel
buffer. Its parameters and buffer sizes therefore change only between batches.
XLyap uses copied job parameters and worker-local orbit state. Its random forcing
mode remains serial so the random sequence and orbit progression are preserved.
Prime UMAP retains its single background pipeline and UMAP's own thread controls.
