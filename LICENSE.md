MUD98 / ROM 2.4 Licensing
=========================

This project contains code from multiple sources with different licensing terms.
The applicable license for any given file or code section is determined by the
hierarchy described below.


LICENSE HIERARCHY
-----------------

1. **File-Specific Licenses** (Highest Priority)
   Some source files contain explicit copyright notices and license grants in
   their headers. When present, these file-specific licenses govern that file.
   
   Examples include:
   - OLC (Online Creation) system files (src/olc/*) - "The Isles 1.1" license
   - Community snippet contributions (src/snippets/*) - Various author licenses
   
   Always check the file header for copyright and license information.

2. **Non-Derivative Original Work** - MIT License
   Code that is original work by the project maintainers and is NOT derived from
   or based upon Diku/Merc/ROM code is licensed under the MIT License (below).
   
   This primarily includes:
   - New subsystems added to the codebase (e.g., Lox scripting, MTH protocol handler)
   - Modern architectural improvements and refactoring
   - New persistence layer abstractions
   - Test and benchmark infrastructure
   - Build system and tooling
   - Documentation not derived from legacy sources
   
   When in doubt, assume code is derivative unless clearly a new subsystem or
   marked with modern copyright notices indicating otherwise.

3. **Derivative Work** - Diku/Merc/ROM Licenses (Base/Default)
   All code derived from or based upon the original Diku Mud, Merc Diku Mud, or
   Rivers of MUD (ROM) 2.4 beta codebases is subject to their respective licenses:
   
   - Diku License: See LICENSE.diku.txt
   - Merc License: See LICENSE.merc.txt  
   - ROM License: See LICENSE.rom.txt
   
   This includes the vast majority of the core MUD engine, commands, game mechanics,
   world building systems, and related functionality that has evolved from those
   original codebases.


MIT LICENSE (For Non-Derivative Original Work)
-----------------------------------------------

Copyright (c) 2024-2025 Brandon Felger and other contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.


SUMMARY OF DIKU/MERC/ROM OBLIGATIONS
-------------------------------------

If you use or distribute this software, you must comply with the Diku, Merc,
and ROM license terms for all derivative code. Key requirements include:

From Diku License:
- No resale or operation for profit
- Original authors' names must appear in login sequence
- The 'credits' command must report original authors
- Must notify Diku creators of operation

From Merc License:
- Copyrights must remain in source
- 'help merc' must report original help text

From ROM License:
- Login message must credit ROM 2.4 beta, copyright 1993-1996 Russ Taylor
- ROM help entry must be readable by all players and unaltered
- Must email rtaylor@efn.org before opening a ROM-based MUD
- Cannot use "Rivers of Mud" or "ROM" in MUD name (derivatives only)
- Cannot collaborate with known license violators

See the individual license files for complete terms and conditions.


COMPLIANCE GUIDANCE
-------------------

When distributing or operating software based on this codebase:

1. Read and comply with LICENSE.diku.txt, LICENSE.merc.txt, and LICENSE.rom.txt
2. Maintain all existing copyright notices in source files
3. Check individual file headers for file-specific license requirements
4. For non-derivative components under MIT license, maintain MIT license notice
5. Clearly document any modifications you make and their licensing
6. When in doubt about a file's license status, treat it as derivative work


QUESTIONS
---------

For licensing questions regarding:
- Diku/Merc/ROM obligations: Refer to the original license files
- MIT-licensed components: Contact the project maintainers
- File-specific licenses: Contact the original file authors where possible
- This project: See the repository documentation or contact maintainers