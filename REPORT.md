# Concurrent Banking System with Scheduling, Synchronization, IPC, and Memory Management

## Objective
This single-file project implements scheduling, synchronization, deadlock avoidance, ipc, and memory management for the required banking simulation.

## Scheduling
- FCFS average waiting time: 4.80
- Priority average waiting time: 5.60
- Round Robin average waiting time: 5.80

Sample FCFS Gantt chart:
```text
| R1 | P1 | V1 | C1 | R2 |
0 4 7 9 14 16
```

## Synchronization
- Shared account initial balance: 1000
- Shared account final balance: 950
- Successful withdrawals: 2
- Failed withdrawals: 0
- Payroll threads: 50
- Observed semaphore limit: 2
- Total payroll paid: 10000

## Banker
- Safe request: request granted for Loan-B.
- Unsafe request: request denied for Loan-A because it leads to an unsafe state.

## IPC
- Requests processed: 4
- Final balances: ACC-100=1300, ACC-200=750

## Memory
- FIFO page faults: 10
- FIFO hit ratio: 0.23
- LRU page faults: 9
- LRU hit ratio: 0.31
