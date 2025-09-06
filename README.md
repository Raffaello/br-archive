# BR-Archive

BR-Archive (BRa) is an educational project to compress files with the `DEFLATE` algorithm and creating self-extracting-archives (SFX)

The Default extension for the archive format is `.BRa`

Wildcard expansion rely on the shell or runtime;  see [Wildcard Expansions](#wildcard-expansions) below.

This lays the foundation for encoding, decoding, and self-extracting archives.

> [!NOTE]
> This project aims to avoid using third-party libraries.


## Self-Extracting Archive

The self extracting archive will be done in the following formats:

### PE/EXE

an `.EXE` SFX will be generated for MS Windows oriented systems.

### ELF

an `.BRx` SFX will be generated for Linux oriented systems.


## Wildcard Expansions

Currently supported wildcards: `*` and `?`.

> Notes:
> - On POSIX shells, the shell may expand patterns; quote them to let BRa expand: `bra '*.br?'`.
> - Examples: `*.txt`, `image_??.png`
> - Directory inputs expand to `dir/*`; empty directories are intentionally not archived when not recursive.

