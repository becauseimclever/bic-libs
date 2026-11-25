/* PIO model adapted from CHIRBot; Copyright (c) 2026 Darren, MIT. */
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "bic/nes_snes.h"
#include "hardware/gpio.h"

enum { LATCH = 2, CLOCK = 0, DATA = 4 };
pio_hw_t test_pio0, test_pio1;
static bool latch, clock_high = true, directions[NUM_BANK0_GPIOS];
static unsigned reads, pulses, serial_bit, disabled_pulls;
static uint16_t pressed;
static uint64_t elapsed;

void gpio_init(uint pin) { directions[pin] = false; }
void gpio_set_pulls(uint pin, bool up, bool down) {
    assert(!up && !down);
    disabled_pulls |= 1u << pin;
}
void gpio_set_dir(uint pin, bool output) { directions[pin] = output; }
void gpio_put(uint pin, bool value) {
    if (pin == LATCH) {
        if (value) { assert(clock_high); serial_bit = 0; }
        latch = value;
    } else {
        assert(pin == CLOCK);
        if (value && !clock_high) { assert(!latch); ++serial_bit; ++pulses; }
        clock_high = value;
    }
}
bool gpio_get(uint pin) {
    assert(pin == DATA && !directions[DATA] && !latch && clock_high && serial_bit < 16);
    ++reads;
    return (pressed & (1u << serial_bit)) == 0;
}
void sleep_us(uint64_t us) {
    assert(us == (latch ? 12u : 6u));
    elapsed += us;
}

/* The model executes the production PIO source one instruction per tick. */
static char instructions[32][40], labels[8][24];
static unsigned label_pc[8], instruction_count, label_count, pc;
static uint32_t x, y, osr, isr, fifo_word;
static bool fifo_full, pio_latch, pio_clock = true, pio_data, inverted, enabled;
static int claim_result, program_result, init_result;
static unsigned unclaims, removals;

static void load_program(const char *path) {
    FILE *f = fopen(path, "r");
    assert(f);
    char line[128];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') ++p;
        p[strcspn(p, "\r\n")] = 0;
        if (!*p || *p == '.' || *p == ';') continue;
        size_t n = strlen(p);
        if (p[n - 1] == ':') {
            assert(label_count < 8 && n < sizeof(labels[0]));
            p[n - 1] = 0;
            memcpy(labels[label_count], p, n);
            label_pc[label_count++] = instruction_count;
        } else {
            assert(instruction_count < 32 && n < sizeof(instructions[0]));
            memcpy(instructions[instruction_count++], p, n + 1);
        }
    }
    fclose(f);
    assert(instruction_count);
}
static unsigned label(const char *name) {
    for (unsigned i = 0; i < label_count; ++i)
        if (!strcmp(labels[i], name)) return label_pc[i];
    assert(!"Unknown PIO label");
    return 0;
}
static void tick(void) {
    if (!enabled) return;
    const char *op = instructions[pc];
    unsigned next = pc + 1;
    if (!strcmp(op, "pull block")) {
        if (!fifo_full) return;
        osr = fifo_word; fifo_full = false;
    } else if (!strcmp(op, "pull noblock")) {
        if (fifo_full) { osr = fifo_word; fifo_full = false; }
        else osr = x;
    } else if (!strcmp(op, "mov x, osr")) x = osr;
    else if (!strcmp(op, "mov isr, null")) isr = 0;
    else if (!strcmp(op, "in pins, 1")) isr = (isr << 1) | pio_clock;
    else if (!strcmp(op, "mov y, isr")) y = isr;
    else if (!strcmp(op, "out pins, 1")) { pio_data = (osr & 1u) != 0; osr >>= 1; }
    else if (!strncmp(op, "jmp pin ", 8)) { if (pio_latch) next = label(op + 8); }
    else if (!strncmp(op, "jmp !y ", 7)) { if (!y) next = label(op + 7); }
    else if (!strncmp(op, "jmp ", 4)) next = label(op + 4);
    else assert(!"Unsupported PIO instruction");
    pc = next == instruction_count ? 0 : next;
}
static void settle(void) { for (unsigned i = 0; i < 64; ++i) tick(); }
static void signals(bool latched, bool clock) { pio_latch = latched; pio_clock = clock; settle(); }
static bool data(void) { return pio_data != inverted; }

uint pio_get_gpio_base(PIO pio) { assert(pio == pio0); return 0; }
int pio_claim_unused_sm(PIO pio, bool required) { assert(pio == pio0 && !required); return claim_result; }
void pio_sm_unclaim(PIO pio, uint sm) { assert(pio == pio0 && sm == 0); ++unclaims; }
int pio_add_program(PIO pio, const pio_program_t *p) { assert(pio == pio0 && p); return program_result; }
void pio_remove_program(PIO pio, const pio_program_t *p, uint offset) {
    assert(pio == pio0 && p && offset == 0); ++removals;
}
void sm_config_set_out_pins(pio_sm_config *c, uint pin, uint count) { assert(count == 1); c->out_pin = pin; }
void sm_config_set_set_pins(pio_sm_config *c, uint pin, uint count) { assert(count == 1); c->set_pin = pin; }
void sm_config_set_out_shift(pio_sm_config *c, bool right, bool auto_pull, uint threshold) {
    assert(!auto_pull && threshold == 32); c->out_right = right;
}
void sm_config_set_in_pins(pio_sm_config *c, uint pin) { c->in_pin = pin; }
void sm_config_set_in_shift(pio_sm_config *c, bool right, bool auto_push, uint threshold) {
    assert(!auto_push && threshold == 32); c->in_right = right;
}
void sm_config_set_jmp_pin(pio_sm_config *c, uint pin) { c->jmp_pin = pin; }
int pio_sm_init(PIO pio, uint sm, uint offset, const pio_sm_config *c) {
    assert(pio == pio0 && sm == 0 && offset == 0);
    assert(c->out_pin == DATA && c->set_pin == DATA && c->in_pin == CLOCK &&
           c->jmp_pin == LATCH && c->out_right && !c->in_right);
    pc = 0; x = y = osr = isr = 0; fifo_full = false;
    return init_result;
}
int pio_sm_set_consecutive_pindirs(PIO pio, uint sm, uint pin, uint count, bool out) {
    assert(pio == pio0 && sm == 0 && pin == DATA && count == 1 && out); return 0;
}
void pio_sm_set_pins_with_mask64(PIO pio, uint sm, uint64_t value, uint64_t mask) {
    assert(pio == pio0 && sm == 0 && mask == (UINT64_C(1) << DATA));
    pio_data = (value & mask) != 0;
}
void gpio_set_inover(uint pin, uint value) { assert(pin < NUM_BANK0_GPIOS && value == GPIO_OVERRIDE_NORMAL); }
void gpio_set_outover(uint pin, uint value) {
    assert(pin < NUM_BANK0_GPIOS);
    if (pin == DATA) inverted = value == GPIO_OVERRIDE_INVERT;
    else assert(value == GPIO_OVERRIDE_NORMAL);
}
void gpio_set_oeover(uint pin, uint value) { assert(pin < NUM_BANK0_GPIOS && value == GPIO_OVERRIDE_NORMAL); }
void pio_gpio_init(PIO pio, uint pin) { assert(pio == pio0 && pin == DATA); }
void pio_sm_put(PIO pio, uint sm, uint32_t value) {
    assert(pio == pio0 && sm == 0 && !fifo_full); fifo_word = value; fifo_full = true;
}
void pio_sm_set_enabled(PIO pio, uint sm, bool value) { assert(pio == pio0 && sm == 0); enabled = value; }
void pio_sm_clear_fifos(PIO pio, uint sm) { assert(pio == pio0 && sm == 0); fifo_full = false; }

static void check_bits(bic_nes_snes_profile_t profile, uint16_t buttons) {
    unsigned width = profile == BIC_NES_SNES_NES ? 8 : 16;
    for (unsigned bit = 0; bit < 40; ++bit) {
        bool expected = bit < width ? !(buttons & (1u << bit)) : profile == BIC_NES_SNES_SNES;
        assert(data() == expected);
        signals(false, false);
        assert(data() == expected);
        signals(false, true);
    }
}
static void reader_tests(void) {
    bic_nes_snes_reader_t reader = BIC_NES_SNES_READER_INIT;
    uint16_t actual = 0;
    assert(bic_nes_snes_reader_read(&reader, &actual) == BIC_NES_SNES_NOT_INITIALIZED);
    assert(bic_nes_snes_reader_init(&reader, BIC_NES_SNES_NES, 2, 2, 4) == BIC_NES_SNES_INVALID_ARGUMENT);
    assert(bic_nes_snes_reader_init(&reader, (bic_nes_snes_profile_t)2, LATCH, CLOCK, DATA) == BIC_NES_SNES_INVALID_ARGUMENT);
    assert(bic_nes_snes_reader_init(&reader, BIC_NES_SNES_NES, LATCH, CLOCK, DATA) == BIC_NES_SNES_OK);
    assert(directions[LATCH] && directions[CLOCK] && !directions[DATA]);
    assert(disabled_pulls == ((1u << LATCH) | (1u << CLOCK) | (1u << DATA)));
    assert(bic_nes_snes_reader_init(&reader, BIC_NES_SNES_NES, LATCH, CLOCK, DATA) == BIC_NES_SNES_ALREADY_INITIALIZED);
    for (unsigned mask = 0; mask <= 255; ++mask) {
        pressed = (uint16_t)mask; reads = pulses = 0; elapsed = 0;
        assert(bic_nes_snes_reader_read(&reader, &actual) == BIC_NES_SNES_OK && actual == pressed);
        assert(reads == 8 && pulses == 8 && elapsed == 114 && !latch && clock_high);
    }
    assert(bic_nes_snes_reader_deinit(&reader) == BIC_NES_SNES_OK);
    assert(!directions[LATCH] && !directions[CLOCK]);
    assert(bic_nes_snes_reader_init(&reader, BIC_NES_SNES_SNES, LATCH, CLOCK, DATA) == BIC_NES_SNES_OK);
    for (unsigned bit = 0; bit < 16; ++bit) {
        pressed = (uint16_t)(1u << bit); reads = pulses = 0; elapsed = 0;
        assert(bic_nes_snes_reader_read(&reader, &actual) == BIC_NES_SNES_OK && actual == pressed);
        assert(reads == 16 && pulses == 16 && elapsed == 210);
    }
    assert(bic_nes_snes_reader_deinit(&reader) == BIC_NES_SNES_OK);
}
static void emulator_tests(void) {
    bic_nes_snes_emulator_t e = BIC_NES_SNES_EMULATOR_INIT;
    assert(bic_nes_snes_emulator_set_buttons(&e, 0) == BIC_NES_SNES_NOT_INITIALIZED);
    assert(bic_nes_snes_emulator_init(&e, BIC_NES_SNES_NES, NULL, LATCH, CLOCK, DATA) == BIC_NES_SNES_INVALID_ARGUMENT);
    claim_result = -1;
    assert(bic_nes_snes_emulator_init(&e, BIC_NES_SNES_NES, pio0, LATCH, CLOCK, DATA) == BIC_NES_SNES_NO_RESOURCES);
    claim_result = 0; program_result = -1;
    assert(bic_nes_snes_emulator_init(&e, BIC_NES_SNES_NES, pio0, LATCH, CLOCK, DATA) == BIC_NES_SNES_NO_RESOURCES && unclaims == 1);
    program_result = 0; init_result = -1;
    assert(bic_nes_snes_emulator_init(&e, BIC_NES_SNES_NES, pio0, LATCH, CLOCK, DATA) == BIC_NES_SNES_HARDWARE_ERROR && unclaims == 2 && removals == 1);
    init_result = 0;
    for (unsigned profile = 0; profile < 2; ++profile) {
        assert(bic_nes_snes_emulator_init(&e, (bic_nes_snes_profile_t)profile, pio0, LATCH, CLOCK, DATA) == BIC_NES_SNES_OK);
        settle();
        assert(data());
        unsigned cases = profile == BIC_NES_SNES_NES ? 256 : 16;
        for (unsigned i = 0; i < cases; ++i) {
            uint16_t buttons = (uint16_t)(profile == BIC_NES_SNES_NES ? i : 1u << i);
            assert(bic_nes_snes_emulator_set_buttons(&e, buttons) == BIC_NES_SNES_OK);
            signals(true, true); signals(false, true);
            check_bits((bic_nes_snes_profile_t)profile, buttons);
        }
        if (profile == BIC_NES_SNES_NES) {
            assert(bic_nes_snes_emulator_set_buttons(&e, 0x100) == BIC_NES_SNES_INVALID_ARGUMENT);
            signals(true, true);
            assert(bic_nes_snes_emulator_set_buttons(&e, 1) == BIC_NES_SNES_OK);
            settle(); assert(!data());
            signals(false, true);
            assert(bic_nes_snes_emulator_set_buttons(&e, 0x80) == BIC_NES_SNES_OK);
            check_bits(BIC_NES_SNES_NES, 1);
            signals(true, true); signals(false, true);
            check_bits(BIC_NES_SNES_NES, 0x80);
            signals(true, true); signals(false, true);
            signals(false, false); signals(false, true);
            signals(false, false);
            signals(true, false); signals(true, true); signals(false, true);
            check_bits(BIC_NES_SNES_NES, 0x80);
            assert(bic_nes_snes_emulator_set_buttons(&e, 0xff) == BIC_NES_SNES_OK);
            assert(bic_nes_snes_emulator_set_buttons(&e, 0) == BIC_NES_SNES_OK);
            signals(true, true); signals(false, true);
            check_bits(BIC_NES_SNES_NES, 0);
        }
        assert(bic_nes_snes_emulator_deinit(&e) == BIC_NES_SNES_OK);
        assert(!enabled && !inverted);
    }
}
int main(int argc, char **argv) {
    assert(argc == 2);
    load_program(argv[1]);
    reader_tests();
    emulator_tests();
    return 0;
}
