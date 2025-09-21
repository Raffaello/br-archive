# BR-Archive

BR-Archive (BRa) is an educational project to compress files with the `DEFLATE` algorithm and creating self-extracting-archives (SFX)

The Default extension for the archive format is `.BRa`

Wildcard expansion rely on the shell or runtime;  see [Wildcard Expansions](#wildcard-expansions) below.

This lays the foundation for encoding, decoding, and self-extracting archives.

> [!NOTE]
> This project aims to avoid using third-party libraries.


## Library

The `lib-bra` is wanted to be in pure C.

The only exception is for the `filesystem` module aspect that is using C++ `filesystem` module to have a simpler cross-os filesystem APIs.
 (do not want to implement for each specific OS).
 The `bra_fs` can be used directly in C++, but `lib-bra` is using the C wrapper `bra-fs_c`.


 ## Entry MAX_LENGTH (Directory & Filename)

 There is a limit for each entry, file or directory, to be max 255 chars.

 Potentially if divided in multiple entry, sub-dirs for e.g., it can overcome that maximum.

 It won't support a filename longer than 255 chars and based on the directory structure at the moment,
 it might fail to encoding as the path is longer than 255 chars (`dir/filename` for e.g.).
 In such a case would be mostly enough to split in 2 entries, but it is not planned yet nor implemented.
 (alternatively would be enough to give a max of 65k chars using a uint16_t)
 

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
