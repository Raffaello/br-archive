# BR-Archive

BR-Archive (BRa) is an educational project to compress files with various algorithm and creating self-extracting-archives (SFX).

The main compression algorithm will be the classical '90s, and later on looking to use the `asymmetric number system` technique. 

----

The Default extension for the archive format is `.BRa`

Wildcard expansion rely on the shell or runtime;  see [Wildcard Expansions](#wildcard-expansions) below.

This lays the foundation for encoding, decoding, and self-extracting archives.

> [!NOTE]
> This project aims to avoid using third-party libraries.


## Library

It is mostly written in C with some exceptions for C++.
final programs are in C++.

The exception is for the `filesystem` module aspect that is using C++ `filesystem` module to have a simpler cross-os filesystem APIs.
> (Not interested for this project to implement FS primitives for each specific OS in plain C).


## Self-Extracting Archive

The self extracting archive will be created in the following formats:

### PE/EXE

an `.EXE` SFX will be generated for MS Windows oriented systems.

### ELF

an `.BRx` SFX will be generated for Linux oriented systems.


## Wildcard Expansions

Currently supported wildcards: `*` and `?`.

> Notes:
> - On POSIX shells and PowerShell, patterns are expanded by the shell; quote them to prevent expansion if you want BRa to receive the pattern literally.
> - On Windows cmd.exe, patterns are expanded by the C runtime (setargv) enabled in `MINGW32` build; use `^` or quotes to prevent expansion.
> - BRa itself only expands directory arguments to `dir/*` (non-recursive); empty directories are intentionally not archived.
> - Examples: `*.txt`, `image_??.png`

## Entry name length limit (single path component)
Each entry name (file or directory) is limited to 255 bytes after UTF‑8 encoding.
The limit applies to a single path component (no parent path) and does not include the trailing `NULL`.

- file is the filename without the directory path
- directory is just the directory name without the parent path.

Entries whose single‑component name exceeds 255 bytes are rejected at pack time and treated as invalid during extraction.
