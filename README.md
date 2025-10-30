# Multi-threaded File Storage Server

A Dropbox-like server with signup/login, per-user storage quotas, and concurrent file operations over TCP. Uses two thread-safe queues (Client Queue and Task Queue) and two thread pools (Client and Worker). Includes a simple interactive client.

## Group

- Husnain Raza (BSCS23076)

## Build

- Build both server and client:
```
make
```
- Run server/client:
```
make run_server
make run_client
```
- Clean:
```
make clean
```

## Sanitizers and Valgrind

- ThreadSanitizer build:
```
make tsan
```
- AddressSanitizer build:
```
make asan
```
- Run under Valgrind:
```
make valgrind_server
make valgrind_client
```

## Architecture

- Main thread: accepts TCP connections and enqueues `client` objects into `ClntQue`.
- Client thread pool: dequeues clients, performs SIGNUP/LOGIN, receives commands; for each command creates a `task` and enqueues into `TaskQue`, then waits on `task` condition variable for completion.
- Worker thread pool: dequeues `task`, performs UPLOAD/DOWNLOAD/DELETE/LIST, does quota checks and file I/O, sets `task->status/resp/resp_len`, signals the waiting client thread.
- File locking: per-path locks via `FileLockTable` to serialize conflicting operations on the same file or directory.

## Synchronization Strategy

- Queues: implemented with a linked-list queue (`queue.c`) protected by a mutex and a condition variable in the thread logic (mutexes/CVs live outside the queue to keep it generic).
- Producer/consumer:
  - Client threads wait on `ClientQueCV` while `ClntQue` is empty; workers wait on `TaskQueCV` while `TaskQue` is empty.
- Worker→Client result delivery:
  - Chosen approach: the client thread creates a `task` with its own `pthread_mutex_t m` and `pthread_cond_t cv`, enqueues it, and waits on `cv`. The worker updates the `task` and signals `cv`. The client thread then sends the response over its socket. This avoids busy waiting and keeps socket I/O in client threads.

## Commands (from client)
- `SIGNUP <username> <password>`
- `LOGIN <username> <password>`
- `UPLOAD <filename>`
- `DOWNLOAD <filename>`
- `DELETE <filename>`
- `LIST`

## Testing

- Interactive test: run server (`make run_server`) and in another terminal run client (`make run_client`), then try the commands.
- Concurrent load test (Python): spawns multiple concurrent sessions.
```
python3 scripts/load_test.py
```

## Notes and Known Warnings
- Format specifiers: code uses `%zu` for `size_t`; ensure matched types to avoid warnings.
- LIST buffer: large directory listings may truncate; consider dynamic buffers for production.
- Paths: server creates per-user directories as `<username><user_id>`; ensure working directory is writable.

## Tools
- SQLite3 used for users and quotas.
- Pthreads for concurrency.
- Valgrind/TSan for memory and race detection.

## Future Improvements
- Priority queue for tasks.
- Optional content encoding for UPLOAD/DOWNLOAD.
- More robust protocol (framing, errors) and TLS.
