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
> - On POSIX shells and PowerShell, patterns are expanded by the shell; quote them to prevent expansion if you want BRa to receive the pattern literally.
> - On Windows cmd.exe, patterns are expanded by the C runtime (setargv) enabled in our build; use ^ or quotes to prevent expansion.
> - BRa itself only expands directory arguments to `dir/*` (non-recursive); empty directories are intentionally not archived.
> - Examples: `*.txt`, `image_??.png`
