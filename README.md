# process-stat

It is a reporting system that pass per-process reosurce usage with a zero-copy API to the users. It may be used in resource monitoring libraries to calculate memory, I/O or cpu load and usages overal or per processes.

## Build

To build the module:

```
make
insmode mmap.ko
```

To build testapp:

```
gcc test.c
```
