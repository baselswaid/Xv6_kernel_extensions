# xv6 Kernel Extensions: System Calls, Containers & Namespaces

Kernel-level work in C on **xv6**, MIT's teaching Unix-like operating system for x86. It consists of three assignments from the Operating Systems course at the Open University of Israel. Each folder contains only the kernel and user files I modified or wrote, on top of the course's xv6 base.

| Folder | Assignment | Main file |
|---|---|---|
| [`01-ps-system-call`](01-ps-system-call) | Add a `ps` system call and user command | `proc.c`, `usys.S`, `syscall.c` |
| [`02-container-aware-ps`](02-container-aware-ps) | Make `ps` work inside containers (PID namespaces) | `proc.c`, `syscall.h` |
| [`03-mount-namespace-fix`](03-mount-namespace-fix) | Fix a kernel panic in mount-namespace teardown | `mnt_ns.c` |

## 1. A `ps` system call, under a hard constraint

I added a new system call (`cps115`) and a `ps` user program that lists every live process with its name, PID, state and parent PID. Kernel output uses `cprintf`, and the process table is walked under `ptable.lock`.

**The constraint:** `syscall.h` could not be modified, so the usual "add a `SYS_` number" route was closed. Instead:
- The handler is registered directly at slot `[22]` of the dispatch table in `syscall.c`.
- A custom `SYSCALLPS` macro in `usys.S` loads `22` into `%eax` and traps into the kernel with `int $T_SYSCALL`, which is the same mechanism the stock `SYSCALL` macro generates.

This required understanding exactly how xv6 gets from a user-space call to the kernel handler: the user stub, the trap, the dispatch table and the return value in `%eax`.

## 2. Container-aware `ps`

The course's xv6 base was extended with **containers**, built on PID and mount namespaces and driven by a `pouch` command. The process structure (`struct proc`) changed to support them, so the old system call no longer compiled.

I rewrote it to be namespace-aware, and this time registered it the conventional way through `syscall.h`. For each process it now prints:

| Column | Meaning |
|---|---|
| `PID` | PID **inside** the process's own namespace |
| `EXTPID` | PID as seen from the root namespace, resolved with `get_pid_for_ns()` |
| `PPID` | Parent's namespace PID |
| `CPUTIME` | CPU time consumed |

## 3. Debugging a kernel panic in mount namespaces

**Symptom:** running the `mounttest` program twice in a row crashed the kernel with `panic: out of mount_list objects`.

**Root cause:** `mount_nsleave()` was effectively empty. When the last process left a mount namespace, the namespace's reference count was never decremented to zero and its mounts were never released. Each run leaked namespace slots and mount objects until the pool ran out.

**Fix**, in [`mnt_ns.c`](03-mount-namespace-fix/mnt_ns.c), under the namespace-table lock:

```c
void mount_nsleave(struct mount_ns* mount_ns)
{
    acquire(&mountnstable.lock);
    if (--mount_ns->ref == 0) {            // last process leaving
        umountlist(mount_ns->list_mounts); // release all of its mounts
        mount_ns->list_mounts = 0;         // mark the slot reusable
    }
    release(&mountnstable.lock);
}
```

After the fix, `allocmount_ns()` can reuse the slot (`ref == 0 && list_mounts == 0`), and repeated runs no longer panic.

## Notes

- These folders are **patches, not a standalone OS**. To build, copy a folder's files over the matching course xv6 base and run `make qemu`.
- xv6 is © MIT PDOS (Frans Kaashoek, Robert Morris, Russ Cox) under the MIT license. The container and namespace extensions come from the course's xv6 base. My work is the code in this repository's folders.
