##
## This file is part of the libsigrokdecode project.
##
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, see <http://www.gnu.org/licenses/>.
##

'''
Bus decoder and disassembler for the WDC W65C02S (CMOS 6502) CPU.

The decoder reconstructs one bus cycle per PHI2 period and turns the
cycles into a disassembly, using SYNC to find the instruction boundaries.

Required signals: D0-D7, PHI2 (the clock input, pin 37 on the DIP), RWB
and SYNC. A0-A15 are optional but strongly recommended: with them the
disassembly shows instruction addresses and absolute branch targets.
VPB, RESB and RDY are optional and improve reset, interrupt and
wait-state handling.

Capture in timing (asynchronous) mode with at least ~10 samples per PHI2
period. The buses are evaluated shortly BEFORE the falling edge of PHI2,
which is the only point where address, data and control lines are all
valid on a 65C02.

The opcode table is the W65C02S one: it includes the Rockwell bit
instructions (RMB/SMB/BBR/BBS), WAI and STP, and treats all undefined
opcodes as the NOPs they are on this chip. Code that only uses documented
NMOS 6502 opcodes decodes correctly too; NMOS "illegal" opcodes do not.
'''

from .pd import Decoder
