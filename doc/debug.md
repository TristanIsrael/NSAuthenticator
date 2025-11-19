# Debug

Analyzing a crash report can be done this way:

```
/opt/devkitpro/devkitA64/bin/aarch64-none-elf-addr2line -e authenticator.elf -f -C [address]
```

