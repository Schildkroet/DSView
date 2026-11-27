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

import sigrokdecode as srd

class Ann:
    ADDR, OPCODE, READ, WRITE, INSTR, EVENT, WARN = range(7)

class Pin:
    D0 = 0                      # D0..D7   = 0..7
    PHI2, RWB, SYNC = 8, 9, 10
    A0 = 11                     # A0..A15  = 11..26
    VPB, RESB, RDY = 27, 28, 29

# Fields of a bus cycle tuple.
SS, ES, ADDR, DATA, RD, SYNC, VEC = range(7)

# Addressing modes: instruction length in bytes and operand format.
# {0} is the first operand byte, {1} the second one.
MODES = {
    'imp': (1, ''),
    'acc': (1, 'A'),
    'imm': (2, '#${0}'),
    'zp':  (2, '${0}'),
    'zpx': (2, '${0},X'),
    'zpy': (2, '${0},Y'),
    'zpi': (2, '(${0})'),
    'izx': (2, '(${0},X)'),
    'izy': (2, '(${0}),Y'),
    'abs': (3, '${1}{0}'),
    'abx': (3, '${1}{0},X'),
    'aby': (3, '${1}{0},Y'),
    'ind': (3, '(${1}{0})'),
    'iax': (3, '(${1}{0},X)'),
    'rel': (2, None),           # relative branch
    'zpr': (3, None),           # BBRn/BBSn: zero page, relative
}

# W65C02S opcode matrix, one line per high nibble. Lower case "nop" marks
# the undefined opcodes, which this chip executes as NOPs of the given size.
OPCODE_MATRIX = '''
BRK/imm ORA/izx nop/imm nop/imp TSB/zp  ORA/zp  ASL/zp  RMB0/zp PHP/imp ORA/imm ASL/acc nop/imp TSB/abs ORA/abs ASL/abs BBR0/zpr
BPL/rel ORA/izy ORA/zpi nop/imp TRB/zp  ORA/zpx ASL/zpx RMB1/zp CLC/imp ORA/aby INC/acc nop/imp TRB/abs ORA/abx ASL/abx BBR1/zpr
JSR/abs AND/izx nop/imm nop/imp BIT/zp  AND/zp  ROL/zp  RMB2/zp PLP/imp AND/imm ROL/acc nop/imp BIT/abs AND/abs ROL/abs BBR2/zpr
BMI/rel AND/izy AND/zpi nop/imp BIT/zpx AND/zpx ROL/zpx RMB3/zp SEC/imp AND/aby DEC/acc nop/imp BIT/abx AND/abx ROL/abx BBR3/zpr
RTI/imp EOR/izx nop/imm nop/imp nop/zp  EOR/zp  LSR/zp  RMB4/zp PHA/imp EOR/imm LSR/acc nop/imp JMP/abs EOR/abs LSR/abs BBR4/zpr
BVC/rel EOR/izy EOR/zpi nop/imp nop/zpx EOR/zpx LSR/zpx RMB5/zp CLI/imp EOR/aby PHY/imp nop/imp nop/abs EOR/abx LSR/abx BBR5/zpr
RTS/imp ADC/izx nop/imm nop/imp STZ/zp  ADC/zp  ROR/zp  RMB6/zp PLA/imp ADC/imm ROR/acc nop/imp JMP/ind ADC/abs ROR/abs BBR6/zpr
BVS/rel ADC/izy ADC/zpi nop/imp STZ/zpx ADC/zpx ROR/zpx RMB7/zp SEI/imp ADC/aby PLY/imp nop/imp JMP/iax ADC/abx ROR/abx BBR7/zpr
BRA/rel STA/izx nop/imm nop/imp STY/zp  STA/zp  STX/zp  SMB0/zp DEY/imp BIT/imm TXA/imp nop/imp STY/abs STA/abs STX/abs BBS0/zpr
BCC/rel STA/izy STA/zpi nop/imp STY/zpx STA/zpx STX/zpy SMB1/zp TYA/imp STA/aby TXS/imp nop/imp STZ/abs STA/abx STZ/abx BBS1/zpr
LDY/imm LDA/izx LDX/imm nop/imp LDY/zp  LDA/zp  LDX/zp  SMB2/zp TAY/imp LDA/imm TAX/imp nop/imp LDY/abs LDA/abs LDX/abs BBS2/zpr
BCS/rel LDA/izy LDA/zpi nop/imp LDY/zpx LDA/zpx LDX/zpy SMB3/zp CLV/imp LDA/aby TSX/imp nop/imp LDY/abx LDA/abx LDX/aby BBS3/zpr
CPY/imm CMP/izx nop/imm nop/imp CPY/zp  CMP/zp  DEC/zp  SMB4/zp INY/imp CMP/imm DEX/imp WAI/imp CPY/abs CMP/abs DEC/abs BBS4/zpr
BNE/rel CMP/izy CMP/zpi nop/imp nop/zpx CMP/zpx DEC/zpx SMB5/zp CLD/imp CMP/aby PHX/imp STP/imp nop/abs CMP/abx DEC/abx BBS5/zpr
CPX/imm SBC/izx nop/imm nop/imp CPX/zp  SBC/zp  INC/zp  SMB6/zp INX/imp SBC/imm NOP/imp nop/imp CPX/abs SBC/abs INC/abs BBS6/zpr
BEQ/rel SBC/izy SBC/zpi nop/imp nop/zpx SBC/zpx INC/zpx SMB7/zp SED/imp SBC/aby PLX/imp nop/imp nop/abs SBC/abx INC/abx BBS7/zpr
'''

def build_opcode_table():
    table = []
    for entry in OPCODE_MATRIX.split():
        mnemonic, mode = entry.split('/')
        # (mnemonic, addressing mode, is an undefined opcode)
        table.append((mnemonic.upper(), mode, mnemonic.islower()))
    assert len(table) == 256
    return table

OPCODES = build_opcode_table()

# Where the operand bytes sit in the cycle sequence of an instruction
# (cycle 0 is the opcode fetch). Only used when the address bus is not
# connected; with A0-A15 the operand fetches are found by their address.
OPERAND_CYCLES = {'JSR': (1, 5), 'BBR': (1, 4), 'BBS': (1, 4)}
DEFAULT_OPERAND_CYCLES = (1, 2)

# Number of cycles after RESB is released in which the reset vector fetch
# has to show up (the reset sequence itself takes 7 cycles).
RESET_SEQ_CYCLES = 12

def hex2(byte):
    return '??' if byte is None else '%02X' % byte

class Decoder(srd.Decoder):
    api_version = 3
    id = 'w65c02'
    name = '65C02'
    longname = 'WDC W65C02S / 6502 CPU bus'
    desc = 'W65C02S (CMOS 6502) bus cycles and instruction disassembly.'
    license = 'gplv2+'
    inputs = ['logic']
    outputs = []
    tags = ['Retro computing']
    # Display hints for DSView; other front ends ignore them.
    pin_on_top = True       # show the decoder above the channels
    fixed_label = True      # always labelled with 'name', whatever a session says
    channel_colours = (
        ('a', 'addr'),      # A0-A15 in the colour of the address bus row
        ('d', 'read'),      # D0-D7 in the colour of data reads (most data cycles)
    )
    annotation_colours = (
        ('addr', '#80c8bd'),    # address bus row
        ('instr', '#c6cbe6'),   # instructions row (resets/interrupts stand out)
    )
    channels = tuple({
            'id': 'd%d' % i,
            'name': 'D%d' % i,
            'desc': 'Data bus line %d' % i
            } for i in range(8)
    ) + (
        {'id': 'phi2', 'name': 'PHI2', 'desc': 'CPU clock input'},
        {'id': 'rwb', 'name': 'RWB', 'desc': 'Read/write, high = read'},
        {'id': 'sync', 'name': 'SYNC', 'desc': 'Opcode fetch cycle'},
    )
    optional_channels = tuple({
            'id': 'a%d' % i,
            'name': 'A%d' % i,
            'desc': 'Address bus line %d' % i
            } for i in range(16)
    ) + (
        {'id': 'vpb', 'name': 'VPB', 'desc': 'Vector pull, active low'},
        {'id': 'resb', 'name': 'RESB', 'desc': 'Reset, active low'},
        {'id': 'rdy', 'name': 'RDY', 'desc': 'Ready, low = CPU halted'},
    )
    options = (
        {'id': 'sample_ns',
            'desc': 'Sample point before PHI2 falling edge (ns)',
            'default': 30},
    )
    annotations = (
        ('addr', 'Address'),
        ('opcode', 'Opcode fetch'),
        ('read', 'Data read'),
        ('write', 'Data write'),
        ('instr', 'Instruction'),
        ('event', 'Reset or interrupt'),
        ('warn', 'Warning'),
    )
    annotation_rows = (
        ('addr-bus', 'Address bus', (Ann.ADDR,)),
        ('data-bus', 'Data bus', (Ann.OPCODE, Ann.READ, Ann.WRITE)),
        ('instrs', 'Instructions', (Ann.INSTR, Ann.EVENT)),
        ('warnings', 'Warnings', (Ann.WARN,)),
    )

    def __init__(self):
        self.reset()

    def reset(self):
        self.samplerate = None
        self.have_addr = False
        self.have_vpb = False
        self.have_resb = False
        self.have_rdy = False
        self.cyc = []           # Bus cycles of the instruction in progress.
        self.in_reset = False   # RESB is currently low.
        self.reset_ss = None
        self.last_samplenum = None
        self.reset_seq = 0      # Cycles left to find the reset vector fetch.
        self.reset_blind = False  # Reset vector found by position only.
        self.hold_ss = None     # Start of a cycle stretched by RDY.
        self.cycle_ss = None    # Start of the bus cycle in progress.

    def metadata(self, key, value):
        if key == srd.SRD_CONF_SAMPLERATE:
            self.samplerate = value

    def start(self):
        self.out_ann = self.register(srd.OUTPUT_ANN)

    def put_ann(self, ss, es, ann, texts):
        self.put(ss, es, self.out_ann, [ann, texts])

    # --- Sampling: one bus cycle per PHI2 period --------------------------

    def margin_samples(self):
        # Number of samples to step back from the PHI2 falling edge.
        if not self.samplerate:
            return 1
        ns = max(0, int(self.options['sample_ns']))
        return max(1, (ns * int(self.samplerate) + 999999999) // 1000000000)

    def decode(self):
        self.have_addr = all(self.has_channel(Pin.A0 + i) for i in range(16))
        self.have_vpb = bool(self.has_channel(Pin.VPB))
        self.have_resb = bool(self.has_channel(Pin.RESB))
        self.have_rdy = bool(self.has_channel(Pin.RDY))
        margin = self.margin_samples()

        # While PHI2 is low nothing of interest happens. While it is high,
        # follow every change of the data bus, so that the bus state from
        # 'margin' samples before the falling edge is known once the edge
        # arrives. Sampling AT the edge would be too late: the 65C02 only
        # holds its data for about 10 ns.
        wait_rise = {Pin.PHI2: 'r'}
        wait_high = [{Pin.PHI2: 'f'}] + [{Pin.D0 + i: 'e'} for i in range(8)]

        while True:
            # Note: DSView hands out the same tuple on every wait() call and
            # refills it in place, so it must be copied and must never be
            # referenced any more when wait() is called again.
            pins = list(self.wait(wait_rise))
            rise = self.samplenum
            # end() closes a reset that is still active with this.
            self.last_samplenum = rise
            history = [(rise, pins)]
            while True:
                pins = list(self.wait(wait_high))
                self.last_samplenum = self.samplenum
                if pins[Pin.PHI2] == 0:
                    break
                history.append((self.samplenum, pins))
            fall = self.samplenum

            pins = history[0][1]
            for samplenum, snapshot in history:
                if samplenum > fall - margin:
                    break
                pins = snapshot

            # A bus cycle runs from one PHI2 falling edge to the next one.
            ss = self.cycle_ss if self.cycle_ss is not None else rise
            self.cycle_ss = fall
            self.handle_cycle(ss, fall, pins)

    def end(self):
        # Called by DSView when the capture has been processed completely.
        last = getattr(self, 'last_samplenum', None)
        if self.in_reset and last is not None and last > self.reset_ss:
            self.put_ann(self.reset_ss, last, Ann.EVENT, ['RESB low', 'RESB', 'R'])
        self.flush(truncated=True)

    # --- Bus cycles -> instructions ---------------------------------------

    def handle_cycle(self, ss, es, pins):
        data = 0
        for bit in reversed(pins[Pin.D0:Pin.D0 + 8]):
            data = (data << 1) | bit
        addr = None
        if self.have_addr:
            addr = 0
            for bit in reversed(pins[Pin.A0:Pin.A0 + 16]):
                addr = (addr << 1) | bit
        rd = pins[Pin.RWB] != 0
        sync = pins[Pin.SYNC] != 0
        vec = (pins[Pin.VPB] == 0) if self.have_vpb else None

        # The '@' prefix marks a number for DSView, which then shows it in
        # the display format selected for the decoder (hex, dec, ascii...).
        if addr is not None:
            self.put_ann(ss, es, Ann.ADDR, ['@%04X' % addr])
        if not rd:
            self.put_ann(ss, es, Ann.WRITE, ['@%02X' % data])
        elif sync:
            self.put_ann(ss, es, Ann.OPCODE, ['@%02X' % data])
        else:
            self.put_ann(ss, es, Ann.READ, ['@%02X' % data])

        # Reset: the bus carries no instructions while RESB is low.
        if self.have_resb and pins[Pin.RESB] == 0:
            if not self.in_reset:
                self.flush(truncated=True)
                self.in_reset = True
                self.reset_ss = ss
            return
        if self.in_reset:
            self.in_reset = False
            self.put_ann(self.reset_ss, ss, Ann.EVENT, ['RESB low', 'RESB', 'R'])
            self.cyc = []
            self.hold_ss = None
            # SYNC is meaningless until the reset vector has been fetched.
            self.reset_seq = RESET_SEQ_CYCLES
            # Without A0-A15 and VPB the vector fetch cannot be seen, but
            # it is always the two cycles before the first opcode fetch.
            self.reset_blind = not (self.have_addr or self.have_vpb)

        # RDY low when the cycle ends: the CPU did not advance, and it
        # repeats this cycle. Merge the repeats into the cycle that counts.
        if self.have_rdy and pins[Pin.RDY] == 0:
            if self.hold_ss is None:
                self.hold_ss = ss
            return
        if self.hold_ss is not None:
            ss = self.hold_ss
            self.hold_ss = None

        cycle = (ss, es, addr, data, rd, sync, vec)

        if self.reset_seq and self.reset_blind:
            # The first cycle of the reset sequence can have SYNC high (the
            # opcode fetch that gets replaced by the reset), so a SYNC only
            # counts once there are cycles for the vector fetch in front.
            self.reset_seq -= 1
            if sync and len(self.cyc) >= 3:
                self.put_reset(self.cyc)
                self.cyc = [cycle]
                self.reset_seq = 0
            elif not self.reset_seq:
                self.cyc = [cycle] if sync else []
            else:
                self.cyc.append(cycle)
            return

        if self.reset_seq:
            self.reset_seq -= 1
            self.cyc.append(cycle)
            if self.is_reset_tail(self.cyc):
                self.put_reset(self.cyc)
                self.cyc = []
                self.reset_seq = 0
            elif not self.reset_seq:
                self.cyc = []
            return

        if sync:
            self.flush()
            self.cyc = [cycle]
        else:
            self.cyc.append(cycle)
            if len(self.cyc) > 24:
                # No SYNC for a long time (STP, WAI, or a wiring problem):
                # keep the head and the tail, drop cycles in the middle.
                del self.cyc[8:16]

    def flush(self, truncated=False):
        cyc = self.cyc
        self.cyc = []
        if not cyc:
            return
        if self.is_reset_tail(cyc):
            # A reset that was not announced by the RESB channel.
            count = min(len(cyc), 7)
            self.put_reset(cyc[-count:])
            cyc = cyc[:-count]
            truncated = True
        if cyc and cyc[0][SYNC]:
            self.put_instruction(cyc, truncated)

    def is_reset_tail(self, cyc):
        # Do the last two cycles fetch the reset vector?
        if len(cyc) < 2:
            return False
        lo, hi = cyc[-2], cyc[-1]
        if not (lo[RD] and hi[RD]):
            return False
        before = cyc[-5:-2]
        if self.have_addr:
            if lo[ADDR] != 0xFFFC or hi[ADDR] != 0xFFFD:
                return False
            if self.have_vpb:
                return lo[VEC] and hi[VEC]
            # Without VPB, the three stack accesses in front of the vector
            # fetch tell a reset from code that merely reads $FFFC/$FFFD,
            # such as JMP ($FFFC).
            return len(before) == 3 and \
                all(c[RD] and (c[ADDR] >> 8) == 0x01 for c in before)
        if self.have_vpb:
            # IRQ, NMI and BRK write to the stack before the vector fetch,
            # reset only reads.
            return lo[VEC] and hi[VEC] and len(before) == 3 and \
                all(c[RD] for c in before)
        return False

    def put_reset(self, cyc):
        target = cyc[-2][DATA] | (cyc[-1][DATA] << 8)
        self.put_ann(cyc[0][SS], cyc[-1][ES], Ann.EVENT,
            ['RESET -> $%04X' % target, 'RESET', 'R'])

    def find_interrupt(self, cyc):
        # Three stack writes in a row only occur when the CPU enters an
        # interrupt (IRQ, NMI or BRK): PCH, PCL, P. Returns the index of
        # the first of these writes, or None.
        for i in range(2, len(cyc) - 4):
            if not (cyc[i][RD] or cyc[i + 1][RD] or cyc[i + 2][RD]):
                return i
        return None

    def operand(self, cyc, pc, mnemonic, num):
        # Get operand byte 'num' (1 or 2) of the instruction in 'cyc'.
        if pc is not None:
            want = (pc + num) & 0xFFFF
            for c in cyc[num:]:
                if c[RD] and c[ADDR] == want:
                    return c[DATA]
            return None
        pos = OPERAND_CYCLES.get(mnemonic[:3], DEFAULT_OPERAND_CYCLES)[num - 1]
        return cyc[pos][DATA] if pos < len(cyc) else None

    def branch_target(self, pc, length, offset):
        if offset is None:
            return '$????'
        if offset & 0x80:
            offset -= 0x100
        if pc is None:
            return '*%+d' % (length + offset)
        return '$%04X' % ((pc + length + offset) & 0xFFFF)

    def put_instruction(self, cyc, truncated):
        ss, es = cyc[0][SS], cyc[-1][ES]
        pc, opcode = cyc[0][ADDR], cyc[0][DATA]

        # Hardware interrupt: the opcode that was fetched is thrown away,
        # and the status register gets pushed with the B flag cleared.
        i = self.find_interrupt(cyc)
        if i is not None and not (cyc[i + 2][DATA] & 0x10):
            vector = cyc[i + 3][ADDR]
            name = {0xFFFA: 'NMI', 0xFFFE: 'IRQ', None: 'IRQ/NMI'}.get(
                vector, 'Interrupt')
            target = cyc[i + 3][DATA] | (cyc[i + 4][DATA] << 8)
            ret = (cyc[i][DATA] << 8) | cyc[i + 1][DATA]
            self.put_ann(ss, es, Ann.EVENT, [
                '%s: PC=$%04X P=$%02X -> $%04X' % (
                    name, ret, cyc[i + 2][DATA], target),
                '%s -> $%04X' % (name, target),
                name, name[0]])
            return

        mnemonic, mode, undefined = OPCODES[opcode]
        length, fmt = MODES[mode]
        ops = [self.operand(cyc, pc, mnemonic, n) for n in range(1, length)]

        if mode == 'rel':
            arg = self.branch_target(pc, 2, ops[0])
        elif mode == 'zpr':
            arg = '$%s,%s' % (hex2(ops[0]), self.branch_target(pc, 3, ops[1]))
        else:
            arg = fmt.format(*[hex2(b) for b in ops])
        asm = mnemonic + ' ' + arg if arg else mnemonic

        texts = []
        if pc is not None:
            texts.append('%04X: %s' % (pc, asm))
        texts.append(asm)
        if arg:
            texts.append(mnemonic)
        if undefined:
            texts.insert(0, '%s (undefined opcode $%02X)' % (texts[0], opcode))
        self.put_ann(ss, es, Ann.INSTR, texts)

        if undefined:
            self.put_ann(ss, es, Ann.WARN,
                ['Undefined opcode $%02X' % opcode, 'Undefined', 'U'])
        if None in ops and not truncated:
            self.put_ann(ss, es, Ann.WARN,
                ['Operand fetch not found (check wiring and SYNC)',
                 'Operand?', '?'])
