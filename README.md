# BR-Archive

BR-Archive (BRa) is an educational project to compress files with the `DEFLATE` algorithm and creating self-extracting-archives (SFX)

The Default extension for the archive format is `.BRa`

This version has partial support for wildcard inputs for file selection;

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
> - Recursive `**` and character classes like `[]` are not supported.
> - Examples: `*.txt`, `image_??.png`
