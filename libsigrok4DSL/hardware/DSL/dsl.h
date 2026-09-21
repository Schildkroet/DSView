/*
 * This file is part of the libsigrok project.
 *
 * Copyright (C) 2013 Bert Vermeulen <bert@biot.com>
 * Copyright (C) 2013 DreamSourceLab <support@dreamsourcelab.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBDSL_HARDWARE_DSL_H
#define LIBDSL_HARDWARE_DSL_H

#include <glib.h>
#include "../../libsigrok-internal.h"
#include "command.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <assert.h>

#include <sys/stat.h>
#include <inttypes.h>


#define USB_INTERFACE		0
#define USB_CONFIGURATION	1
#define NUM_TRIGGER_STAGES	16
#define NUM_SIMUL_TRANSFERS	64
/*
 * How many idle receive_data() ticks pass before the device is polled for
 * capture progress. The tick itself is dsl_get_timeout(), 20 ms in buffer mode,
 * so the poll period is (limit + 1) * 20 ms.
 *
 * 16 is 340 ms, which over a sub-second capture yields two updates - the
 * progress bar reads 0, then ~50, then 100. DSO_E8-1 needs a much shorter
 * period for the bar to track at all, but each poll costs a control-transfer
 * pair on a bus that is already carrying the capture, so DreamSourceLab
 * hardware keeps the original period. Only dscope.c chooses between the two
 * (see e8_empty_poll_limit()); dslogic.c, which DSO_E8-1 never reaches, uses
 * MAX_EMPTY_POLL directly.
 */
#define MAX_EMPTY_POLL      16
#define MAX_EMPTY_POLL_E8   2

#define DSL_REQUIRED_VERSION_MAJOR	2
#define DSL_REQUIRED_VERSION_MINOR	0
#define DSL_HDL_VERSION             0x0E

#define CAPS_FEATURE_NONE 0
// voltage threshold
#define CAPS_FEATURE_VTH (1 << 0)
// with external buffer
#define CAPS_FEATURE_BUF (1 << 1)
// pre offset control
#define CAPS_FEATURE_PREOFF (1 << 2)
// small startup eemprom
#define CAPS_FEATURE_SEEP (1 << 3)
// zero calibration ability
#define CAPS_FEATURE_ZERO (1 << 4)
// use HMCAD1511 adc chip
#define CAPS_FEATURE_HMCAD1511 (1 << 5)
// usb 3.0
#define CAPS_FEATURE_USB30 (1 << 6)
// pogopin panel
#define CAPS_FEATURE_POGOPIN (1 << 7)
// use ADF4360-7 vco chip
#define CAPS_FEATURE_ADF4360 (1 << 8)
// 20M bandwidth limitation
#define CAPS_FEATURE_20M (1 << 9)
// use startup flash (fx3)
#define CAPS_FEATURE_FLASH (1 << 10)
// 32 channels
#define CAPS_FEATURE_LA_CH32 (1 << 11)
// auto tunning vgain
#define CAPS_FEATURE_AUTO_VGAIN (1 << 12)
// max 2.5v fpga threshold
#define CAPS_FEATURE_MAX25_VTH (1 << 13)
// security check
#define CAPS_FEATURE_SECURITY (1 << 14)

/**
 * The device performs its own zero/offset calibration in firmware.
 *
 * dso_zero() here drives a VGA, per-channel preoff registers and a comb
 * compensation table. A device without that hardware can never satisfy the
 * loop, so it would spin forever and the Auto Calibration dialog would wait
 * with it. For such a device the routine completes immediately and the stored
 * result is whatever the firmware itself arrived at.
 *
 * Not an upstream DreamSourceLab capability - added for DSO_E8-1.
 */
#define CAPS_FEATURE_SELF_ZERO (1 << 15)
/* end */


#define DSLOGIC_ATOMIC_BITS 6
#define DSLOGIC_ATOMIC_SAMPLES (1 << DSLOGIC_ATOMIC_BITS)
#define DSLOGIC_ATOMIC_SIZE (1 << (DSLOGIC_ATOMIC_BITS - 3))
#define DSLOGIC_ATOMIC_MASK (0xFFFF << DSLOGIC_ATOMIC_BITS)

/*
 * for basic configuration
 */
#define TRIG_EN_BIT 0       // 0: instant capture, 1: triggers effective
#define CLK_TYPE_BIT 1      // 0: internal clock, 1: external clock
#define CLK_EDGE_BIT 2      // 0: rising edge, 1: falling edge
#define RLE_MODE_BIT 3      // 0: RLE disable, 1: RLE enable (only effective when required)
#define DSO_MODE_BIT 4      // 0: logic mode, 1: DSO mode
#define HALF_MODE_BIT 5     // 0: normal sample rate, 1: double sample rate with halved channels (only effective when required)
#define QUAR_MODE_BIT 6     // 0: normal sample rate, 1: quadruple sample rate with quartered channels (only effective when required)
#define ANALOG_MODE_BIT 7   // 0: digital mode, 1: analog mode
#define FILTER_BIT 8        // 0: no filter, 1: 1T filter
#define INSTANT_BIT 9       // ? does not affect instant acquisition, TRIG_EN_BIT (0) is used for that
#define SLOW_ACQ_BIT 10     // 0: normal acquisition, 1: slow acquisition when bytes per ms < 1024
#define STRIG_MODE_BIT 11   // 0: normal trigger, 1: serial trigger
#define STREAM_MODE_BIT 12  // 0: buffer mode, 1: stream mode
#define LPB_TEST_BIT 13     // 0: normal mode, 1: loopback test mode (?)
#define EXT_TEST_BIT 14     // 0: normal mode, 1: external test mode (?)
#define INT_TEST_BIT 15     // 0: normal mode, 1: internal test mode (channels return binary counter)

#define bmNONE          0
#define bmEEWP          (1 << 0) // EEPROM write protect disable
#define bmFORCE_RDY     (1 << 1) // get device ready for next acquisition
#define bmFORCE_STOP    (1 << 2) // stop acquisition now, prepare for data readout
#define bmSCOPE_SET     (1 << 3) // enable scope mode
#define bmSCOPE_CLR     (1 << 4) // disable scope mode
#define bmBW20M_SET     (1 << 5) // enable 20M bandwidth limit
#define bmBW20M_CLR     (1 << 6) // disable 20M bandwidth limit

/*
 * packet content check
 */
#define TRIG_CHECKID 0x55555555
#define DSO_PKTID 0xa500

/*
 * zero configuration
 */
#define DSO_ZERO_PAGE 8
#define MAX_ACC_VARIANCE 0.0005
/*
 * for DSCope device
 * trans: x << 8 + y
 * x = vpos(coarse), each step(1024 total) indicate x(mv) at 1/20 attenuation, and x/10(mv) at 1/2 attenuation
 * y = voff(fine), each step(1024 total) indicate y/100(mv) at 1/20 attenuation, adn y/1000(mv) at 1/2 attenuation
 * voff: x << 10 + y
 * x = vpos(coarse) default bias
 * y = voff(fine) default bias
 * the final offset: x+DSCOPE_CONSTANT_BIAS->vpos(coarse); y->voff(fine)
 */
#define DSCOPE_CONSTANT_BIAS 160
#define DSCOPE_TRANS_CMULTI   10
#define DSCOPE_TRANS_FMULTI   100.0

/*
 * for DSCope20 device
 * trans: the whole windows offset map to the offset pwm(1024 total)
 * voff: offset pwm constant bias to balance circuit offset
 */
#define CALI_VGAIN_RANGE 600

struct DSL_caps {
    uint64_t mode_caps;
    uint64_t feature_caps;
    uint64_t channels;
    uint64_t total_ch_num;
    uint64_t hw_depth;
    uint64_t dso_depth;
    uint8_t intest_channel;
    const uint64_t *vdivs;
    const uint64_t *samplerates;
    uint8_t vga_id;
    uint16_t default_channelmode;
    uint64_t default_samplerate;
    uint64_t default_samplelimit;
    uint16_t default_pwmtrans;
    uint16_t default_pwmmargin;
    uint32_t ref_min;
    uint32_t ref_max;
    uint16_t default_comb_comp;
    uint64_t half_samplerate;
    uint64_t quarter_samplerate;
};

struct DSL_profile {
    uint16_t vid;
    uint16_t pid;
    enum libusb_speed usb_speed;

    const char *vendor;
    const char *model; //product name
    const char *model_version;

    const char *firmware;

    const char *fpga_bit33;
    const char *fpga_bit50;

    struct DSL_caps dev_caps;
};

static const uint64_t vdivs10to2000[] = {
    SR_mV(10),
    SR_mV(20),
    SR_mV(50),
    SR_mV(100),
    SR_mV(200),
    SR_mV(500),
    SR_V(1),
    SR_V(2),
    0,
};

static const uint64_t samplerates100[] = {
    SR_HZ(10),
    SR_HZ(20),
    SR_HZ(50),
    SR_HZ(100),
    SR_HZ(200),
    SR_HZ(500),
    SR_KHZ(1),
    SR_KHZ(2),
    SR_KHZ(5),
    SR_KHZ(10),
    SR_KHZ(20),
    SR_KHZ(40),
    SR_KHZ(50),
    SR_KHZ(100),
    SR_KHZ(200),
    SR_KHZ(400),
    SR_KHZ(500),
    SR_MHZ(1),
    SR_MHZ(2),
    SR_MHZ(4),
    SR_MHZ(5),
    SR_MHZ(10),
    SR_MHZ(20),
    SR_MHZ(25),
    SR_MHZ(50),
    SR_MHZ(100),
    0,
};

static const uint64_t samplerates400[] = {
    SR_HZ(10),
    SR_HZ(20),
    SR_HZ(50),
    SR_HZ(100),
    SR_HZ(200),
    SR_HZ(500),
    SR_KHZ(1),
    SR_KHZ(2),
    SR_KHZ(5),
    SR_KHZ(10),
    SR_KHZ(20),
    SR_KHZ(40),
    SR_KHZ(50),
    SR_KHZ(100),
    SR_KHZ(200),
    SR_KHZ(400),
    SR_KHZ(500),
    SR_MHZ(1),
    SR_MHZ(2),
    SR_MHZ(4),
    SR_MHZ(5),
    SR_MHZ(10),
    SR_MHZ(20),
    SR_MHZ(25),
    SR_MHZ(50),
    SR_MHZ(100),
    SR_MHZ(200),
    SR_MHZ(400),
    0,
};

/*
 * DSO_E8-1: the only rates MCO1 can actually produce.
 *
 * PIXCLK is MCO1 looped back through the front end, and MCO1 is an integer
 * divide (1..15) of one of PLL1Q 192 MHz / HSI 64 / HSI48 48 / HSE 24. Offering
 * anything else would let the GUI pick a rate the hardware silently rounds,
 * making the timebase wrong with no indication.
 *
 * Each entry also maps to a UNIQUE ceil(max_samplerate / rate / channels)
 * divider for both 1- and 2-channel configurations, which is how the firmware
 * recovers the requested rate from the settings block.
 *
 * 8..32 MHz. The top is the DSO front end's maximum sample rate, not an MCO1
 * limit - it can reach 48, 64 and 96 MHz, but the analog path cannot follow.
 * The bottom is a floor on how long a capture may take: at 8 MHz a full 8 MiB
 * record already runs 1.05 s, and slower rates stretch that far enough that the
 * host starts treating the device as unresponsive.
 */
/*
 * DSO_E8-1 vertical ranges.
 *
 * These are the volts per division the front end ACTUALLY delivers, not round
 * nominal values: its gain is 1.1x what a straight reading of the ADC's code
 * range would imply, so a 1 V signal on a nominal 1 V/div range would read as
 * 1.1 V. The entry is nominal/1.1.
 *
 * Why here and not in ref_min/ref_max, which look like the natural place: that
 * pair defines the CODE domain, and DsoSignal::ratio2value() maps the vertical
 * position and trigger level through it. Widening the span past 0..255 to
 * absorb the gain would push both out of the range the 8-bit samples can
 * express. The volts-per-division list is the honest place for a gain that the
 * analog path cannot trim away.
 *
 * ONE range, because the board has no variable gain - only the attenuator
 * (x1 / x10 / x100), and that is driven by DSView's probe-factor buttons (see
 * e8_set_attenuator() in dscope.c). DSView multiplies the displayed V/div by
 * the selected factor, so this single x1 range reads as 182 mV, 1.82 V and
 * 18.2 V per division at x1, x10 and x100 - each one the hardware really
 * delivers. DSView converts samples to volts through V/div, so any further
 * range would only relabel the same samples wrongly.
 *
 * At x1 the 2.0 V ADC span over DS_CONF_DSO_VDIVS (10) divisions is 200 mV/div
 * nominal, i.e. 200 / 1.1 = 181.8 -> 182 mV/div after the front end's 1.1 gain
 * (0.1% rounding; integer mV). That assumes the gain holds at x1 - verify with a
 * known amplitude at 182 mV/div. Re-derive if the gain ever changes, and keep
 * vga_defaults[]'s DSL_VGA_ID_E8 key equal to this entry.
 */
static const uint64_t vdivs_dso_e8[] = {
    SR_mV(218),     /* x1; the probe factor scales it to 1.82 V / 18.2 V (nominal 200 mV / 1.1) */
    0,
};

static const uint64_t samplerates_dso_e8[] = {
    SR_MHZ(2),
    SR_MHZ(4),
    SR_MHZ(8),
    SR_MHZ(12),
    SR_MHZ(16),
    SR_MHZ(24),
    SR_MHZ(32),
    0,
};

/*
 * DSO_E8-1 logic mode: samplerates_dso_e8[] plus 64 MHz (MCO1 = PLL1Q 192 / 3).
 * The 32 MHz ceiling there belongs to the analog front end, which the logic
 * probes bypass. Picked by dsl_samplerates() for this device in a LOGIC channel
 * mode. MUST match DSL_LOGIC_SAMPLE_RATES in the firmware's DSL_Cfg.h.
 */
static const uint64_t samplerates_logic_e8[] = {
    SR_MHZ(2),
    SR_MHZ(4),
    SR_MHZ(8),
    SR_MHZ(12),
    SR_MHZ(16),
    SR_MHZ(24),
    SR_MHZ(32),
    SR_MHZ(64),
    0,
};

static const uint64_t samplerates1000[] = {
    SR_HZ(10),
    SR_HZ(20),
    SR_HZ(50),
    SR_HZ(100),
    SR_HZ(200),
    SR_HZ(500),
    SR_KHZ(1),
    SR_KHZ(2),
    SR_KHZ(5),
    SR_KHZ(10),
    SR_KHZ(20),
    SR_KHZ(40),
    SR_KHZ(50),
    SR_KHZ(100),
    SR_KHZ(200),
    SR_KHZ(400),
    SR_KHZ(500),
    SR_MHZ(1),
    SR_MHZ(2),
    SR_MHZ(4),
    SR_MHZ(5),
    SR_MHZ(10),
    SR_MHZ(20),
    SR_MHZ(25),
    SR_MHZ(50),
    SR_MHZ(100),
    SR_MHZ(125),
    SR_MHZ(250),
    SR_MHZ(500),
    SR_GHZ(1),
    0,
};

struct DSL_vga {
    uint8_t id;
    uint64_t key;
    uint64_t vgain;
    uint16_t preoff;
    uint16_t preoff_comp;
};
/* DSO_E8-1's USB identity. dsl_check_conf_profile() accepts these strings
 * instead of DreamSourceLab's, but only for this VID:PID - every other device
 * still has to present the original manufacturer/product strings. Must match
 * string_desc_arr[] in the firmware's HAL/USB/usb_descriptors.c. */
#define DSL_E8_VID          0x2A0E
#define DSL_E8_PID          0x00E8
#define DSL_E8_MANUFACTURER "Hermelin Labs"
#define DSL_E8_PRODUCT      "Hermelin MSO-E8"

/* DSO_E8-1's VGA table id - chosen well clear of the DreamSourceLab ids (1..5). */
#define DSL_VGA_ID_E8 0xE8

/* DSO_E8-1's zero code: the sample value 0 V lands on. The board has no
 * programmable offset (it ignores SR_CONF_PROBE_OFFSET), so this is fixed -
 * unlike a DSCope, whose hardware moves the signal to the GUI's offset. Must
 * equal the hw_offset the firmware reports in every status block
 * (DSL_MSTAT_HW_OFFSET), or instant and continuous captures disagree. */
#define DSL_E8_ZERO_CODE 128

/* DSO_E8-1 logic capture depth, in SAMPLES.
 *
 * The board captures the 8 DCMI data lines into SDRAM as one byte per sample -
 * all eight lines at once, so the capacity does not change with how many
 * channels are enabled in the GUI. It is the firmware's LOGIC_BUFFER_SIZE
 * (Libraries/Logic/Logic_Cfg.h), which is DCMI_CAPTURE_MAX_BYTES, 8 MiB.
 *
 * hw_depth cannot carry this: it is shared with DSO mode, where DSView divides
 * it by unit_bits and the enabled channel count, and the value that lands on
 * 8 Mi samples there (SR_MB(64)) overstates the logic duration list by 8x -
 * offering 2.1 s at 32 MHz against a real 262 ms. Everything past the real
 * depth is zero padding on the trace. */
#define DSL_E8_LOGIC_DEPTH (8 * 1024 * 1024)

static const struct DSL_vga vga_defaults[] = {
    {1, 10,   0x162400, (32<<10)+558, (32<<10)+558},
    {1, 20,   0x14C000, (32<<10)+558, (32<<10)+558},
    {1, 50,   0x12E800, (32<<10)+558, (32<<10)+558},
    {1, 100,  0x118000, (32<<10)+558, (32<<10)+558},
    {1, 200,  0x102400, (32<<10)+558, (32<<10)+558},
    {1, 500,  0x2E800,  (32<<10)+558, (32<<10)+558},
    {1, 1000, 0x18000,  (32<<10)+558, (32<<10)+558},
    {1, 2000, 0x02400,  (32<<10)+558, (32<<10)+558},

    {2, 10,   0x1DA800, 45, 1024-920-45},
    {2, 20,   0x1A7200, 45, 1024-920-45},
    {2, 50,   0x164200, 45, 1024-920-45},
    {2, 100,  0x131800, 45, 1024-920-45},
    {2, 200,  0xBD000,  45, 1024-920-45},
    {2, 500,  0x7AD00,  45, 1024-920-45},
    {2, 1000, 0x48800,  45, 1024-920-45},
    {2, 2000, 0x12000,  45, 1024-920-45},

    {3, 10,   0x1C5C00, 45, 1024-920-45},
    {3, 20,   0x19EB00, 45, 1024-920-45},
    {3, 50,   0x16AE00, 45, 1024-920-45},
    {3, 100,  0x143D00, 45, 1024-920-45},
    {3, 200,  0xB1000,  45, 1024-920-45},
    {3, 500,  0x7F000,  45, 1024-920-45},
    {3, 1000, 0x57200,  45, 1024-920-45},
    {3, 2000, 0x2DD00,  45, 1024-920-45},

    {4, 10,   0x1C6C00, 60, 1024-900-60},
    {4, 20,   0x19E000, 60, 1024-900-60},
    {4, 50,   0x16A800, 60, 1024-900-60},
    {4, 100,  0x142800, 60, 1024-900-60},
    {4, 200,  0xC7F00,  60, 1024-900-60},
    {4, 500,  0x94000,  60, 1024-900-60},
    {4, 1000, 0x6CF00,  60, 1024-900-60},
    {4, 2000, 0x44F00,  60, 1024-900-60},

    {5, 10,   0x1C3400, 60, 1024-900-60},
    {5, 20,   0x19BD00, 60, 1024-900-60},
    {5, 50,   0x167400, 60, 1024-900-60},
    {5, 100,  0x13F300, 60, 1024-900-60},
    {5, 200,  0xC4F00,  60, 1024-900-60},
    {5, 500,  0x91B00,  60, 1024-900-60},
    {5, 1000, 0x69D00,  60, 1024-900-60},
    {5, 2000, 0x41D00,  60, 1024-900-60},

    /* DSO_E8-1: no VGA, and V/div no longer moves hardware - the attenuator
     * follows the probe-factor buttons instead (e8_set_attenuator()). One
     * entry for its one range, so dso_vga()'s lookup still resolves. */
    {DSL_VGA_ID_E8, 182,   0, 0, 0},   /* key == vdivs_dso_e8[0] */
    {0, 0, 0, 0, 0}
};

struct DSL_channels {
    enum DSL_CHANNEL_ID id;
    enum OPERATION_MODE mode;
    enum CHANNEL_TYPE type;
    gboolean stream;
    uint16_t num;
    uint16_t vld_num;
    uint8_t unit_bits;
    uint64_t min_samplerate;
    uint64_t max_samplerate;
    uint64_t hw_min_samplerate;
    uint64_t hw_max_samplerate;
    uint8_t pre_div;
    const char *descr;
};

static const struct DSL_channels channel_modes[] = {
    // LA Stream
    {DSL_STREAM20x16,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE,  16, 16, 1, SR_KHZ(10), SR_MHZ(20),
        SR_KHZ(10), SR_MHZ(100), 1, "Use 16 Channels (Max 20MHz)"},
    {DSL_STREAM25x12,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE,  16, 12, 1, SR_KHZ(10), SR_MHZ(25),
        SR_KHZ(10), SR_MHZ(100), 1, "Use 12 Channels (Max 25MHz)"},
    {DSL_STREAM50x6,   LOGIC,  SR_CHANNEL_LOGIC,  TRUE,  16, 6,  1, SR_KHZ(10), SR_MHZ(50),
        SR_KHZ(10), SR_MHZ(100), 1, "Use 6 Channels (Max 50MHz)"},
    {DSL_STREAM100x3,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE,  16, 3,  1, SR_KHZ(10), SR_MHZ(100),
        SR_KHZ(10), SR_MHZ(100), 1, "Use 3 Channels (Max 100MHz)"},

    {DSL_STREAM20x16_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 16, 1, SR_KHZ(10), SR_MHZ(20),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 16 Channels (Max 20MHz)"},
    {DSL_STREAM25x12_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 12, 1, SR_KHZ(10), SR_MHZ(25),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 12 Channels (Max 25MHz)"},
    {DSL_STREAM50x6_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 6, 1, SR_KHZ(10), SR_MHZ(50),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 6 Channels (Max 50MHz)"},
    {DSL_STREAM100x3_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 3, 1, SR_KHZ(10), SR_MHZ(100),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 3 Channels (Max 100MHz)"},

    {DSL_STREAM10x32_32_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 32, 1, SR_KHZ(10), SR_MHZ(10),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 32 Channels (Max 10MHz)"},
    {DSL_STREAM20x16_32_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 16, 1, SR_KHZ(10), SR_MHZ(20),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 16 Channels (Max 20MHz)"},
    {DSL_STREAM25x12_32_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 12, 1, SR_KHZ(10), SR_MHZ(25),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 12 Channels (Max 25MHz)"},
    {DSL_STREAM50x6_32_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 6, 1, SR_KHZ(10), SR_MHZ(50),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 6 Channels (Max 50MHz)"},
    {DSL_STREAM100x3_32_3DN2,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 3, 1, SR_KHZ(10), SR_MHZ(100),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 3 Channels (Max 100MHz)"},

    {DSL_STREAM50x32,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 32, 1, SR_KHZ(10), SR_MHZ(50),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 32 Channels (Max 50MHz)"},
    {DSL_STREAM100x30,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 30, 1, SR_KHZ(10), SR_MHZ(100),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 30 Channels (Max 100MHz)"},
    {DSL_STREAM250x12,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 32, 12, 1, SR_KHZ(10), SR_MHZ(250),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 12 Channels (Max 250MHz)"},
    {DSL_STREAM125x16_16,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 16, 1, SR_KHZ(10), SR_MHZ(125),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 16 Channels (Max 125MHz)"},
    {DSL_STREAM250x12_16,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 16, 12, 1, SR_KHZ(10), SR_MHZ(250),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 12 Channels (Max 250MHz)"},
    {DSL_STREAM500x6,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE,  16, 6,  1, SR_KHZ(10), SR_MHZ(500),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 6 Channels (Max 500MHz)"},
    {DSL_STREAM1000x3,  LOGIC,  SR_CHANNEL_LOGIC,  TRUE, 8, 3,  1, SR_KHZ(10), SR_GHZ(1),
        SR_KHZ(10), SR_MHZ(500), 5, "Use 3 Channels (Max 1GHz)"},

    // LA Buffer
    {DSL_BUFFER100x16, LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 16, 16, 1, SR_KHZ(10), SR_MHZ(100),
        SR_KHZ(10), SR_MHZ(100), 1, "Use Channels 0~15 (Max 100MHz)"},
    {DSL_BUFFER200x8,  LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 8, 8,  1, SR_KHZ(10), SR_MHZ(200),
        SR_KHZ(10), SR_MHZ(100), 1, "Use Channels 0~7 (Max 200MHz)"},
    {DSL_BUFFER400x4,  LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 4, 4,  1, SR_KHZ(10), SR_MHZ(400),
        SR_KHZ(10), SR_MHZ(100), 1, "Use Channels 0~3 (Max 400MHz)"},

    {DSL_BUFFER250x32,  LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 32, 32,  1, SR_KHZ(10), SR_MHZ(250),
        SR_KHZ(10), SR_MHZ(500), 5, "Use Channels 0~31 (Max 250MHz)"},
    {DSL_BUFFER500x16,  LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 16, 16,  1, SR_KHZ(10), SR_MHZ(500),
        SR_KHZ(10), SR_MHZ(500), 5, "Use Channels 0~15 (Max 500MHz)"},
    {DSL_BUFFER1000x8,  LOGIC,  SR_CHANNEL_LOGIC,  FALSE, 8, 8,  1, SR_KHZ(10), SR_GHZ(1),
        SR_KHZ(10), SR_MHZ(500), 5, "Use Channels 0~7 (Max 1GHz)"},

    // DAQ
    {DSL_ANALOG10x2,   ANALOG, SR_CHANNEL_ANALOG, TRUE,  2, 2,  8, SR_HZ(10),  SR_MHZ(10),
        SR_KHZ(10), SR_MHZ(100), 1, "Use Channels 0~1 (Max 10MHz)"},
    {DSL_ANALOG10x2_500,   ANALOG, SR_CHANNEL_ANALOG, TRUE,  2, 2,  8, SR_HZ(10),  SR_MHZ(10),
        SR_KHZ(10), SR_MHZ(500), 1, "Use Channels 0~1 (Max 10MHz)"},

    // OSC
    {DSL_DSO200x2,     DSO,    SR_CHANNEL_DSO,    FALSE, 2, 2,  8, SR_KHZ(10), SR_MHZ(200),
        SR_KHZ(10), SR_MHZ(100), 1, "Use Channels 0~1 (Max 200MHz)"},
    {DSL_DSO1000x2,    DSO,    SR_CHANNEL_DSO,    FALSE, 2, 2,  8, SR_KHZ(10), SR_GHZ(1),
        SR_KHZ(10), SR_MHZ(500), 1, "Use Channels 0~1 (Max 1GHz)"}
};

/* hardware Capabilities */
#define CAPS_MODE_LOGIC     (1 << 0)
#define CAPS_MODE_ANALOG    (1 << 1)
#define CAPS_MODE_DSO       (1 << 2)

static const struct DSL_profile supported_DSLogic[] = {
    /*
     * DSLogic
     */
    {DS_VENDOR_ID, 0x0001, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic", NULL,
     "DSLogic.fw",
     "DSLogic33.bin",
     "DSLogic50.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_SEEP | CAPS_FEATURE_BUF,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4) |
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      16,
      SR_MB(256),
      SR_Mn(2),
      DSL_BUFFER100x16,
      vdivs10to2000,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {DS_VENDOR_ID, 0x0003, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic Pro", NULL,
     "DSLogicPro.fw",
     "DSLogicPro.bin",
     "DSLogicPro.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_SEEP | CAPS_FEATURE_VTH | CAPS_FEATURE_BUF,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4),
      16,
      SR_MB(256),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {DS_VENDOR_ID, 0x0020, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic PLus", NULL,
     "DSLogicPlus.fw",
     "DSLogicPlus.bin",
     "DSLogicPlus.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4),
      16,
      SR_MB(256),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {DS_VENDOR_ID, 0x0021, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic Basic", NULL,
     "DSLogicBasic.fw",
     "DSLogicBasic.bin",
     "DSLogicBasic.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4),
      16,
      SR_KB(256),
      0,
      DSL_STREAM20x16,
      0,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {DS_VENDOR_ID, 0x0029, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U2Basic", NULL,
     "DSLogicU2Basic.fw",
     "DSLogicU2Basic.bin",
     "DSLogicU2Basic.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16),
      16,
      SR_MB(64),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates100,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {DS_VENDOR_ID, 0x002A, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U3Pro16", NULL,
     "DSLogicU3Pro16.fw",
     "DSLogicU3Pro16.bin",
     "DSLogicU3Pro16.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_USB30  | CAPS_FEATURE_ADF4360,
      (1 << DSL_STREAM20x16_3DN2) | (1 << DSL_STREAM25x12_3DN2) | (1 << DSL_STREAM50x6_3DN2) | (1 << DSL_STREAM100x3_3DN2) |
      (1 << DSL_BUFFER500x16) | (1 << DSL_BUFFER1000x8),
      16,
      SR_GB(2),
      0,
      DSL_BUFFER500x16,
      0,
      samplerates1000,
      0,
      DSL_STREAM20x16_3DN2,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(500),
      SR_GHZ(1)}
    },

    {DS_VENDOR_ID, 0x002A, LIBUSB_SPEED_SUPER, "DreamSourceLab", "DSLogic U3Pro16", NULL,
     "DSLogicU3Pro16.fw",
     "DSLogicU3Pro16.bin",
     "DSLogicU3Pro16.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_USB30 | CAPS_FEATURE_ADF4360,
      (1 << DSL_STREAM125x16_16) | (1 << DSL_STREAM250x12_16) | (1 << DSL_STREAM500x6) | (1 << DSL_STREAM1000x3) |
      (1 << DSL_BUFFER500x16) | (1 << DSL_BUFFER1000x8),
      16,
      SR_GB(2),
      0,
      DSL_BUFFER500x16,
      0,
      samplerates1000,
      0,
      DSL_STREAM125x16_16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(500),
      SR_GHZ(1)}
    },

    {DS_VENDOR_ID, 0x002C, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U3Pro32", NULL,
     "DSLogicU3Pro32.fw",
     "DSLogicU3Pro32.bin",
     "DSLogicU3Pro32.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_USB30  | CAPS_FEATURE_ADF4360 | CAPS_FEATURE_LA_CH32,
      (1 << DSL_STREAM10x32_32_3DN2) | (1 << DSL_STREAM20x16_32_3DN2) | (1 << DSL_STREAM25x12_32_3DN2) | (1 << DSL_STREAM50x6_32_3DN2) | (1 << DSL_STREAM100x3_32_3DN2) |
      (1 << DSL_BUFFER250x32) | (1 << DSL_BUFFER500x16) | (1 << DSL_BUFFER1000x8),
      32,
      SR_GB(2),
      0,
      DSL_BUFFER250x32,
      0,
      samplerates1000,
      0,
      DSL_STREAM10x32_32_3DN2,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(500),
      SR_GHZ(1)}
    },

    {DS_VENDOR_ID, 0x002C, LIBUSB_SPEED_SUPER, "DreamSourceLab", "DSLogic U3Pro32", NULL,
     "DSLogicU3Pro32.fw",
     "DSLogicU3Pro32.bin",
     "DSLogicU3Pro32.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_USB30 | CAPS_FEATURE_ADF4360 | CAPS_FEATURE_LA_CH32,
      (1 << DSL_STREAM50x32) | (1 << DSL_STREAM100x30) | (1 << DSL_STREAM250x12) | (1 << DSL_STREAM500x6) | (1 << DSL_STREAM1000x3) |
      (1 << DSL_BUFFER250x32) | (1 << DSL_BUFFER500x16) | (1 << DSL_BUFFER1000x8),
      32,
      SR_GB(2),
      0,
      DSL_BUFFER250x32,
      0,
      samplerates1000,
      0,
      DSL_STREAM50x32,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(500),
      SR_GHZ(1)}
    },

    {DS_VENDOR_ID, 0x002D, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U2Pro16", NULL,
     "DSLogicU2Pro16.fw",
     "DSLogicU2Pro16.bin",
     "DSLogicU2Pro16.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_ADF4360 | CAPS_FEATURE_SECURITY,
      (1 << DSL_STREAM20x16_3DN2) | (1 << DSL_STREAM25x12_3DN2) | (1 << DSL_STREAM50x6_3DN2) | (1 << DSL_STREAM100x3_3DN2) |
      (1 << DSL_BUFFER500x16) | (1 << DSL_BUFFER1000x8),
      16,
      SR_GB(4),
      0,
      DSL_BUFFER500x16,
      0,
      samplerates1000,
      0,
      DSL_STREAM20x16_3DN2,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(500),
      SR_GHZ(1)}
    },

    {0x2A0E, 0x0030, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic PLus", NULL,
     "DSLogicPlus.fw",
     "DSLogicPlus-pgl12.bin",
     "DSLogicPlus-pgl12.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_MAX25_VTH | CAPS_FEATURE_SECURITY,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4),
      16,
      SR_MB(256),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {0x2A0E, 0x0031, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U2Basic", NULL,
     "DSLogicU2Basic.fw",
     "DSLogicU2Basic-pgl12.bin",
     "DSLogicU2Basic-pgl12.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_MAX25_VTH | CAPS_FEATURE_SECURITY,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16),
      16,
      SR_MB(64),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates100,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {0x2A0E, 0x0034, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic PLus", NULL,
     "DSLogicPlus-pgl12-2.fw",
     "DSLogicPlus-pgl12-2.bin",
     "DSLogicPlus-pgl12-2.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_MAX25_VTH | CAPS_FEATURE_SECURITY,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16) | (1 << DSL_BUFFER200x8) | (1 << DSL_BUFFER400x4),
      16,
      SR_MB(256),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates400,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },

    {0x2A0E, 0x0035, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSLogic U2Basic", NULL,
     "DSLogicU2Basic-pgl12-2.fw",
     "DSLogicU2Basic-pgl12-2.bin",
     "DSLogicU2Basic-pgl12-2.bin",
     {CAPS_MODE_LOGIC,
      CAPS_FEATURE_VTH | CAPS_FEATURE_BUF | CAPS_FEATURE_MAX25_VTH | CAPS_FEATURE_SECURITY,
      (1 << DSL_STREAM20x16) | (1 << DSL_STREAM25x12) | (1 << DSL_STREAM50x6) | (1 << DSL_STREAM100x3) |
      (1 << DSL_BUFFER100x16),
      16,
      SR_MB(64),
      0,
      DSL_BUFFER100x16,
      0,
      samplerates100,
      0,
      DSL_STREAM20x16,
      SR_MHZ(1),
      SR_Mn(1),
      0,
      0,
      0,
      0,
      0,
      SR_MHZ(200),
      SR_MHZ(400)}
    },


    { 0, 0, LIBUSB_SPEED_UNKNOWN, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};

static const struct DSL_profile supported_DSCope[] = {
    /*
     * DSCope
     */
    {DS_VENDOR_ID, 0x0002, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope", NULL,
     "DSCope.fw",
     "DSCope.bin",
     "DSCope.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_PREOFF | CAPS_FEATURE_SEEP | CAPS_FEATURE_BUF,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      1,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      (129<<8)+167,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0004, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope20", NULL,
     "DSCope20.fw",
     "DSCope20.bin",
     "DSCope20.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_SEEP | CAPS_FEATURE_BUF,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      2,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      920,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0022, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope B20", NULL,
     "DSCopeB20.fw",
     "DSCope20.bin",
     "DSCope20.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_BUF,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      2,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      920,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0023, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope C20", NULL,
     "DSCopeC20.fw",
     "DSCopeC20P.bin",
     "DSCopeC20P.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_BUF,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      3,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      920,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },


    {DS_VENDOR_ID, 0x0024, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope C20P", NULL,
     "DSCopeC20P.fw",
     "DSCopeC20P.bin",
     "DSCopeC20P.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_BUF | CAPS_FEATURE_POGOPIN,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      3,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      920,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0025, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope C20", NULL,
     "DSCopeC20B.fw",
     "DSCopeC20B.bin",
     "DSCopeC20B.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_KB(256),
      SR_Kn(20),
      0,
      vdivs10to2000,
      samplerates400,
      3,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Kn(10),
      920,
      1024-920,
      1,
      255,
      0,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0026, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope U2B20", NULL,
     "DSCopeU2B20.fw",
     "DSCopeU2B20.bin",
     "DSCopeU2B20.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_AUTO_VGAIN,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_KB(256),
      SR_Kn(20),
      0,
      vdivs10to2000,
      samplerates400,
      4,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Kn(10),
      930,
      1024-930,
      10,
      245,
      22,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0027, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope U2P20", NULL,
     "DSCopeU2P20.fw",
     "DSCopeU2P20.bin",
     "DSCopeU2P20.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_BUF | CAPS_FEATURE_POGOPIN | CAPS_FEATURE_AUTO_VGAIN,
      (1 << DSL_ANALOG10x2) |
      (1 << DSL_DSO200x2),
      2,
      SR_MB(256),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates400,
      4,
      DSL_DSO200x2,
      SR_MHZ(100),
      SR_Mn(1),
      930,
      1024-930,
      10,
      245,
      22,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x0028, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope U2B100", NULL,
     "DSCopeU2B100.fw",
     "DSCopeU2B100.bin",
     "DSCopeU2B100.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_HMCAD1511 | CAPS_FEATURE_20M,
      (1 << DSL_ANALOG10x2_500) |
      (1 << DSL_DSO1000x2),
      2,
      SR_KB(256),
      SR_Kn(20),
      0,
      vdivs10to2000,
      samplerates1000,
      4,
      DSL_DSO1000x2,
      SR_MHZ(500),
      SR_Kn(10),
      835,
      1024-835,
      10,
      245,
      60,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x002B, LIBUSB_SPEED_HIGH, "DreamSourceLab", "DSCope U3P100", NULL,
     "DSCopeU3P100.fw",
     "DSCopeU3P100.bin",
     "DSCopeU3P100.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_POGOPIN | CAPS_FEATURE_FLASH | CAPS_FEATURE_USB30 | CAPS_FEATURE_HMCAD1511 | CAPS_FEATURE_20M,
      (1 << DSL_ANALOG10x2_500) |
      (1 << DSL_DSO1000x2),
      2,
      SR_GB(2),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates1000,
      5,
      DSL_DSO1000x2,
      SR_MHZ(500),
      SR_Mn(1),
      780,
      1024-780,
      10,
      245,
      60,
      SR_HZ(0),
      SR_HZ(0)}
    },

    {DS_VENDOR_ID, 0x002B, LIBUSB_SPEED_SUPER, "DreamSourceLab", "DSCope U3P100", NULL,
     "DSCopeU3P100.fw",
     "DSCopeU3P100.bin",
     "DSCopeU3P100.bin",
     {CAPS_MODE_ANALOG | CAPS_MODE_DSO,
      CAPS_FEATURE_ZERO | CAPS_FEATURE_POGOPIN | CAPS_FEATURE_FLASH | CAPS_FEATURE_USB30 | CAPS_FEATURE_HMCAD1511 | CAPS_FEATURE_20M,
      (1 << DSL_ANALOG10x2_500) |
      (1 << DSL_DSO1000x2),
      2,
      SR_GB(2),
      SR_Mn(2),
      0,
      vdivs10to2000,
      samplerates1000,
      5,
      DSL_DSO1000x2,
      SR_MHZ(500),
      SR_Mn(1),
      780,
      1024-780,
      10,
      245,
      60,
      SR_HZ(0),
      SR_HZ(0)}
    },

    /*
     * DSO_E8-1 - Hermelin Labs STM32H750 instrument, not DreamSourceLab hardware.
     *
     * Runs its own firmware (project DSO_E8-1, Libraries/DSL) speaking this
     * protocol directly: an STM32H750 with an AD9280 front end and 8 logic
     * probes, no FX2 and no FPGA. It borrows DreamSourceLab's vendor ID with a
     * PID outside their product range - fine for a one-off board, but it means
     * this entry must never be sent upstream.
     *
     * Consequences of there being no FPGA and no FX2:
     *   - firmware/fpga_bit33/fpga_bit50 are NULL. The device reports
     *     bmFPGA_DONE from DSL_CTL_HW_STATUS, so dsl_dev_open() takes the
     *     "already configured" branch and never looks for those files.
     *   - The logic threshold (SR_CONF_VTH) is a plain VTH_ADDR register write
     *     that the firmware turns into DAC channel C; it needs no feature bit.
     *   - CAPS_FEATURE_BUF only: no SEEP/NVM, no security block, no
     *     ADF4360. Advertising those would send the open sequence off talking
     *     to hardware that is not there.
     *
     * The board has ONE analog channel, but DSL_DSO200x2 is a two-channel mode
     * and the firmware pads CH1 to mid-scale. A dedicated single-channel mode
     * would be more honest; it would also need a new channel_modes[] entry and
     * an audit of whatever in the UI assumes DSO channels come in pairs.
     */
    {0x2A0E, 0x00E8, LIBUSB_SPEED_HIGH, "Hermelin Labs", "MSO-E8", NULL,
     NULL,
     NULL,
     NULL,
     /* LOGIC: the 8 DCMI data lines as logic probes, served by dscope.c's
      * DSO_E8-1 logic branch (see SR_CONF_DEVICE_MODE there) in
      * DSL_BUFFER200x8 - capture to SDRAM, then upload. */
     {CAPS_MODE_DSO | CAPS_MODE_LOGIC,
      /* ZERO exposes the auto-calibration option (SR_CONF_HAVE_ZERO) and 20M the
       * bandwidth-limit one (SR_CONF_BANDWIDTH); without them DSView hides both
       * for this device. The board backs both: the firmware intercepts zero mode
       * to run its own routine, and drives PB8 for the bandwidth filter. */
      CAPS_FEATURE_BUF | CAPS_FEATURE_ZERO | CAPS_FEATURE_20M | CAPS_FEATURE_SELF_ZERO,
      (1 << DSL_DSO200x2) | (1 << DSL_BUFFER200x8),
      2,
      /* hw_depth: bounds the GUI's sample-depth list via
       *     limit_samples = hw_depth / unit_bits / enabled_channels
       * Sized to the board's real acquisition limit, DCMI_CAPTURE_MAX_BYTES
       * (8 MiB, reached with the DMA's double-buffer mode). This must track
       * that constant: advertising more than the hardware can capture leaves
       * the excess as padding, which shows up as a trace that is mostly flat. */
      SR_MB(64),
      /* dso_depth: the size of ONE instant-mode ("single trigger") USB
       * transfer, NOT a capacity. get_buffer_size() returns it verbatim and
       * receive_transfer() resubmits that same buffer until the acquisition's
       * bytes have all arrived, so it is also the granularity of the transfer
       * progress bar - Viewport::get_captured_progress() steps once per
       * completed transfer.
       *
       * Matching it to the full 8 MiB acquisition meant everything arrived in
       * a single transfer, so the bar only ever painted 100%. 256 KiB gives 32
       * steps. DSCope U3P100 does the same: SR_Mn(2) against a depth three
       * orders of magnitude larger.
       *
       * Chunking is invisible to the firmware - it streams one byte sequence
       * into the bulk endpoint either way, and instant mode parses no status
       * block out of the individual transfers (receive_transfer() synthesises
       * mstatus itself and skips get_measure() until the tail).
       */
      262144,
      0,
      vdivs_dso_e8,
      samplerates_dso_e8,  /* only what MCO1 can actually generate */
      DSL_VGA_ID_E8,       /* V/div -> attenuator code, see vga_defaults[] */
      DSL_DSO200x2,
      SR_MHZ(16),       /* MCO1 power-on default = PLL1Q 192 MHz / 12 */
      SR_Mn(1),
      930,
      1024-930,
      10,
      245,
      22,
      SR_HZ(0),
      SR_HZ(0)}
    },
    { 0, 0, LIBUSB_SPEED_UNKNOWN, 0, 0, 0, 0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}
};



enum {
    DSL_ERROR = -1,
    DSL_INIT = 0,
    DSL_START = 1,
    DSL_READY = 2,
    DSL_TRIGGERED = 3,
    DSL_DATA = 4,
    DSL_STOP = 5,
    DSL_FINISH = 7,
    DSL_ABORT = 8,
};

struct DSL_context {
    const struct DSL_profile *profile;
	/*
     * Since we can't keep track of an DSL device after upgrading
	 * the firmware (it renumerates into a different device address
	 * after the upgrade) this is like a global lock. No device will open
	 * until a proper delay after the last device was upgraded.
	 */
	int64_t fw_updated;

	/* Device/capture settings */
	uint64_t cur_samplerate;
	uint64_t limit_samples;
    uint64_t actual_samples;
    uint64_t actual_bytes;

	/* Operational settings */
    gboolean clock_type;
    gboolean clock_edge;
    gboolean rle_mode;
    gboolean rle_support;
    gboolean instant;
    uint16_t op_mode;
    gboolean stream;
    uint8_t  test_mode;
    uint16_t buf_options;
    enum DSL_CHANNEL_ID ch_mode;
    uint16_t samplerates_min_index;
    uint16_t samplerates_max_index;
    uint16_t th_level;
    double vth;
    uint16_t filter;
	uint16_t trigger_mask[NUM_TRIGGER_STAGES];
	uint16_t trigger_value[NUM_TRIGGER_STAGES];
	int trigger_stage;
	uint16_t trigger_buffer[NUM_TRIGGER_STAGES];
    uint64_t timebase;
    uint8_t max_height;
    uint8_t trigger_channel;
    uint8_t trigger_slope;
    uint8_t trigger_source;
    uint8_t trigger_hrate;
    uint32_t trigger_hpos;
    uint64_t trigger_holdoff;
    uint8_t trigger_margin;
    gboolean zero;
    gboolean cali;
    gboolean tune;
    int16_t tune_index;
    int zero_stage;
    int zero_pcnt;
    gboolean zero_branch;
    gboolean zero_comb_fgain;
    gboolean zero_comb;
    int tune_stage;
    int tune_pcnt;
    struct sr_channel *tune_probe;
    gboolean roll;
    gboolean data_lock;
    uint16_t unit_pitch;

    uint64_t num_samples;
    uint64_t num_bytes;
	int submitted_transfers;
	int empty_transfer_count;
    int instant_tail_bytes;

	void *cb_data;
	unsigned int num_transfers;
	struct libusb_transfer **transfers;
	int *usbfd;

    int pipe_fds[2];
    GIOChannel *channel;

    int status;
    int trf_completed;
    gboolean mstatus_valid;
    struct sr_status mstatus;
    gboolean abort;
    gboolean overflow;
    int bw_limit;
    int empty_poll_count;

    int is_loop;

    uint64_t ext_samplerate;

    GThread *usb_thread;
    gboolean usb_thread_quit;
};

/*
 * hardware setting for each capture
 */
struct DSL_setting {
    uint32_t sync;

    uint16_t mode_header;                   // 0
    uint16_t mode;
    uint16_t divider_header;                // 1-2
    uint16_t div_l;
    uint16_t div_h;
    uint16_t count_header;                  // 3-4
    uint16_t cnt_l;
    uint16_t cnt_h;
    uint16_t trig_pos_header;               // 5-6
    uint16_t tpos_l;
    uint16_t tpos_h;
    uint16_t trig_glb_header;               // 7
    uint16_t trig_glb;
    uint16_t dso_count_header;              // 8-9
    uint16_t dso_cnt_l;
    uint16_t dso_cnt_h;
    uint16_t ch_en_header;                  // 10-11
    uint16_t ch_en_l;
    uint16_t ch_en_h;
    uint16_t fgain_header;                  // 12
    uint16_t fgain;
    uint16_t trig_header;                   // 64
    uint16_t trig_mask0[NUM_TRIGGER_STAGES];
    uint16_t trig_mask1[NUM_TRIGGER_STAGES];
    uint16_t trig_value0[NUM_TRIGGER_STAGES];
    uint16_t trig_value1[NUM_TRIGGER_STAGES];
    uint16_t trig_edge0[NUM_TRIGGER_STAGES];
    uint16_t trig_edge1[NUM_TRIGGER_STAGES];
    uint16_t trig_logic0[NUM_TRIGGER_STAGES];
    uint16_t trig_logic1[NUM_TRIGGER_STAGES];
    uint32_t trig_count[NUM_TRIGGER_STAGES];

    uint32_t end_sync;
};

struct DSL_setting_ext32 {
    uint32_t sync;

    uint16_t trig_header;                   // 96
    uint16_t trig_mask0[NUM_TRIGGER_STAGES];
    uint16_t trig_mask1[NUM_TRIGGER_STAGES];
    uint16_t trig_value0[NUM_TRIGGER_STAGES];
    uint16_t trig_value1[NUM_TRIGGER_STAGES];
    uint16_t trig_edge0[NUM_TRIGGER_STAGES];
    uint16_t trig_edge1[NUM_TRIGGER_STAGES];

    uint16_t align_bytes;
    uint32_t end_sync;
};

struct DSL_adc_config {
    uint8_t dest;
    uint8_t cnt;
    uint8_t delay;
    uint8_t byte[4];
};
static const struct DSL_adc_config adc_single_ch0[] = {
    {ADCC_ADDR+1, 3,   0,   {0x03, 0x01, 0x00, 0x00}}, // reset & power down
    {ADCC_ADDR,   4,   0,   {0x00, 0x01, 0x00, 0x31}}, // 1x channel 1/1 clock
    {ADCC_ADDR+1, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x02, 0x3A}}, // adc0: ch0 adc1: ch0
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x02, 0x3B}}, // adc2: ch0 adc3: ch0
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x42}}, // phase_ddr: 270 deg
    {ADCC_ADDR,   4,   0,   {0x00, 0x34, 0x00, 0x50}}, // adc core current: -40% (lower performance) / VCM: +-700uA
    {ADCC_ADDR,   4,   0,   {0x00, 0x22, 0x02, 0x11}}, // lvds drive strength: 1.5mA(RSDS)
    {ADCC_ADDR,   4,   0,   {0x00, 0x7F, 0x00, 0x24}}, // invert all channel
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x55}}, // full-scale range: -10%
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x00, 0x33}}, // x-gain disabled / fine-gain enabled
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x03, 0x2B}}, // coarse_gain: 3dB(1.4125x)
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_single_ch3[] = {
    {ADCC_ADDR+1, 3,   0,   {0x03, 0x01, 0x00, 0x00}}, // reset & power down
    {ADCC_ADDR,   4,   0,   {0x00, 0x01, 0x00, 0x31}}, // 1x channel 1/1 clock
    {ADCC_ADDR+1, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x10, 0x3A}}, // adc0: ch3 adc1: ch3
    {ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x10, 0x3B}}, // adc2: ch3 adc3: ch3
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x42}}, // phase_ddr: 270 deg
    {ADCC_ADDR,   4,   0,   {0x00, 0x34, 0x00, 0x50}}, // adc core current: -40% (lower performance) / VCM: +-700uA
    {ADCC_ADDR,   4,   0,   {0x00, 0x22, 0x02, 0x11}}, // lvds drive strength: 1.5mA(RSDS)
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x24}}, // invert none channel
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x55}}, // full-scale range: -10%
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x00, 0x33}}, // x-gain disabled / fine-gain enabled
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x03, 0x2B}}, // coarse_gain: 3dB(1.4125x)
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_dual_ch03[] = {
    {ADCC_ADDR+1, 3,   0,   {0x03, 0x01, 0x00, 0x00}}, // reset & power down
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x01, 0x31}}, // 2x channel 1/2 clock
    {ADCC_ADDR+1, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x02, 0x3A}}, // adc0: ch0 adc1: ch0
    {ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x10, 0x3B}}, // adc2: ch3 adc3: ch3
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x42}}, // phase_ddr: 270 deg
    {ADCC_ADDR,   4,   0,   {0x00, 0x34, 0x00, 0x50}}, // adc core current: -40% (lower performance) / VCM: +-700uA
    {ADCC_ADDR,   4,   0,   {0x00, 0x22, 0x02, 0x11}}, // lvds drive strength: 1.5mA(RSDS)
    {ADCC_ADDR,   4,   0,   {0x00, 0x11, 0x00, 0x24}}, // invert 0 channel
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x55}}, // full-scale range: -10%
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x00, 0x33}}, // x-gain disabled / fine-gain enabled
    {ADCC_ADDR,   4,   0,   {0x00, 0x33, 0x00, 0x2B}}, // coarse_gain: 3dB(1.4125x)
//    {ADCC_ADDR,   4,   0,   {0x00, 0x03, 0x00, 0x33}}, // x-gain enabled / fine-gain enabled
//    {ADCC_ADDR,   4,   0,   {0x00, 0x11, 0x00, 0x2B}}, // coarse_gain: 1.25x
    //{ADCC_ADDR,   4,   0,   {0x00, 0x40, 0x00, 0x34}}, // fine_gain: 1.0077x
    //{ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x60, 0x35}}, // fine_gain: 1.0077x
    //{ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x00, 0x25}}, // fix pattern test
    //{ADCC_ADDR,   4,   0,   {0x00, 0x00, 0xab, 0x26}}, // test pattern
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_init_fix[] = {
    {ADCC_ADDR+1, 3,   0,   {0x03, 0x01, 0x00, 0x00}}, // reset & power down
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x01, 0x31}}, // 2x channel 1/2 clock
    {ADCC_ADDR+1, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x00, 0x02, 0x02, 0x3A}}, // adc0: ch0 adc1: ch0
    {ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x10, 0x3B}}, // adc2: ch3 adc3: ch3
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x00, 0x42}}, // phase_ddr: 270 deg
    {ADCC_ADDR,   4,   0,   {0x00, 0x34, 0x00, 0x50}}, // adc core current: -40% (lower performance) / VCM: +-700uA
    {ADCC_ADDR,   4,   0,   {0x00, 0x22, 0x02, 0x11}}, // lvds drive strength: 1.5mA(RSDS)
    {ADCC_ADDR,   4,   0,   {0x00, 0x10, 0x00, 0x25}}, // fix pattern test
    {ADCC_ADDR,   4,   0,   {0x00, 0x00, 0x55, 0x26}}, // test pattern
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_clk_init_1g[] = {
    {ADCC_ADDR+2, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x01, 0x61, 0x00, 0x30}}, //
    {ADCC_ADDR,   4,   0,   {0x01, 0x40, 0xF1, 0x46}}, //
    {ADCC_ADDR,   4,   10,  {0x01, 0x62, 0x3D, 0x00}}, //
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_clk_init_500m[] = {
    {ADCC_ADDR+2, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // power up
    {ADCC_ADDR,   4,   0,   {0x01, 0x61, 0x00, 0x30}}, //
    {ADCC_ADDR,   4,   0,   {0x01, 0x40, 0xF1, 0x46}}, //
    {ADCC_ADDR,   4,   10,  {0x01, 0x62, 0x3D, 0x40}}, //
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_power_down[] = {
    //{ADCC_ADDR+2, 1,   0,   {0x00, 0x00, 0x00, 0x00}}, // ADC_CLK power down
    {ADCC_ADDR+1, 1,   0,   {0x00, 0x00, 0x00, 0x00}}, // ADC power down
    {0, 0, 0, {0, 0, 0, 0}}
};
static const struct DSL_adc_config adc_power_up[] = {
    {ADCC_ADDR+1, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // ADC power up
    //{ADCC_ADDR+2, 1,   0,   {0x01, 0x00, 0x00, 0x00}}, // ADC_CLK power up
    {0, 0, 0, {0, 0, 0, 0}}
};

SR_PRIV int dsl_adjust_probes(struct sr_dev_inst *sdi, int num_probes);
SR_PRIV int dsl_setup_probes(struct sr_dev_inst *sdi, int num_probes);
SR_PRIV const GSList *dsl_mode_list(const struct sr_dev_inst *sdi);
SR_PRIV const uint64_t *dsl_samplerates(const struct DSL_context *devc);
SR_PRIV void dsl_adjust_samplerate(struct DSL_context *devc);

SR_PRIV int dsl_en_ch_num(const struct sr_dev_inst *sdi);
SR_PRIV gboolean dsl_check_conf_profile(libusb_device *dev);
SR_PRIV int dsl_configure_probes(const struct sr_dev_inst *sdi);
SR_PRIV uint64_t dsl_channel_depth(const struct sr_dev_inst *sdi);

SR_PRIV int dsl_wr_reg(const struct sr_dev_inst *sdi, uint8_t addr, uint8_t value);
SR_PRIV int dsl_rd_reg(const struct sr_dev_inst *sdi, uint8_t addr, uint8_t *value);
SR_PRIV int dsl_wr_ext(const struct sr_dev_inst *sdi, uint8_t addr, uint8_t value);
SR_PRIV int dsl_rd_ext(const struct sr_dev_inst *sdi, unsigned char *ctx, uint16_t addr, uint8_t len);

SR_PRIV int dsl_wr_dso(const struct sr_dev_inst *sdi, uint64_t cmd);
SR_PRIV int dsl_wr_nvm(const struct sr_dev_inst *sdi, unsigned char *ctx, uint16_t addr, uint8_t len);
SR_PRIV int dsl_rd_nvm(const struct sr_dev_inst *sdi, unsigned char *ctx, uint16_t addr, uint8_t len);
SR_PRIV int dsl_rd_probe(const struct sr_dev_inst *sdi, unsigned char *ctx, uint16_t addr, uint8_t len);

SR_PRIV int dsl_config_adc(const struct sr_dev_inst *sdi, const struct DSL_adc_config *config);
SR_PRIV double dsl_adc_code2fgain(uint8_t code);
SR_PRIV uint8_t dsl_adc_fgain2code(double gain);
SR_PRIV int dsl_config_adc_fgain(const struct sr_dev_inst *sdi, uint8_t branch, double gain0, double gain1);
SR_PRIV int dsl_config_fpga_fgain(const struct sr_dev_inst *sdi);
SR_PRIV int dsl_skew_fpga_fgain(const struct sr_dev_inst *sdi, gboolean comb, double skew[]);
SR_PRIV int dsl_probe_cali_fgain(struct DSL_context *devc, struct sr_channel *probe, double mean, gboolean comb, gboolean reset);
SR_PRIV gboolean dsl_probe_fgain_inrange(struct sr_channel *probe, gboolean comb, double skew[]);

SR_PRIV int dsl_fpga_arm(const struct sr_dev_inst *sdi);
SR_PRIV int dsl_fpga_config(struct libusb_device_handle *hdl, const char *filename);

SR_PRIV int dsl_config_get(int id, GVariant **data, const struct sr_dev_inst *sdi,
                      const struct sr_channel *ch,
                      const struct sr_channel_group *cg);
SR_PRIV int dsl_config_set(int id, GVariant *data, struct sr_dev_inst *sdi,
                      struct sr_channel *ch,
                      struct sr_channel_group *cg );
SR_PRIV int dsl_config_list(int key, GVariant **data, const struct sr_dev_inst *sdi,
                       const struct sr_channel_group *cg);

SR_PRIV int dsl_dev_open(struct sr_dev_driver *di, struct sr_dev_inst *sdi, gboolean *fpga_done);
SR_PRIV int dsl_dev_close(struct sr_dev_inst *sdi);
SR_PRIV int dsl_dev_acquisition_stop(const struct sr_dev_inst *sdi, void *cb_data);
SR_PRIV int dsl_dev_status_get(const struct sr_dev_inst *sdi, struct sr_status *status, gboolean prg);

SR_PRIV unsigned int dsl_get_timeout(const struct sr_dev_inst *sdi);
SR_PRIV int dsl_start_transfers(const struct sr_dev_inst *sdi);
SR_PRIV int dsl_header_size(const struct DSL_context *devc);

SR_PRIV int dsl_destroy_device(struct sr_dev_inst *sdi);

SR_PRIV int dsl_secuCheck(const struct sr_dev_inst *sdi, uint16_t* encryption, int steps);

#endif
