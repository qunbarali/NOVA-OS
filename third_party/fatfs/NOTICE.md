# FatFs third-party notice

FatFs R0.16 source files are vendored from the rusefi/rusefi repository at commit 4e068e8a73e94f9922c2f0907667d8ba77f9bfc9. Original author and copyright holder: ChaN, copyright (C) 2025.

The vendored configuration has been adapted for NOVA: LFN is enabled with a 255-character working limit, the LFN buffer uses static storage, and FatFs internal reentrancy is disabled because NOVA serializes all filesystem calls with its filesystem mutex and exclusive storage-owner gate. NOVA supplies its own SD block-device glue.

## FatFs license

FatFs is an open source software. Redistribution and use of FatFs in source and binary forms, with or without modification, are permitted provided that the following condition is met:

1. Redistributions of source code must retain the above copyright notice, this condition and the following disclaimer.

This software is provided by the copyright holder and contributors "AS IS" and any warranties related to this software are DISCLAIMED. The copyright owner or contributors be NOT LIABLE for any damages caused by use of this software.

