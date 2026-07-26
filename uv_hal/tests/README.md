# uv_hal unit tests

Host-side unit tests for the hardware independent parts of uv_hal.

uv_hal is shared by every Usevolt device, so a regression in it ships to all of
them at once and is usually silent — a filter that settles to the wrong value or
a container that quietly loses an element looks like a hardware fault in the
field, not like a software bug. These tests exist to catch that class of change
at the desk instead of on a machine.

## Running

```bash
make            # build and run everything
make build      # build only
make san        # build and run under AddressSanitizer + UBSanitizer
make clean
```

Run a subset by passing a substring filter, matched against `suite.test_name`:

```bash
make run T=hysteresis
./build/uv_hal_tests pid
./build/uv_hal_tests json_reader.reads_strings
```

The runner exits non-zero if anything fails, so it drops straight into CI.

Run `make san` before committing a change to any of the modules under test.
Plain assertions cannot see a one byte overrun on a fixed size buffer;
AddressSanitizer can, and that is the defect class that actually bites on a
Cortex-M with no MMU. It has already found one out of bounds write here.

## Adding a test

Add a `TEST(suite, name) { ... }` block to any `test_*.c` file, or create a new
`test_*.c` — the makefile globs them and test cases register themselves, so
nothing else needs touching.

```c
TEST(hysteresis, triggers_above_the_trigger_value) {
	uv_hysteresis_st hyst;
	uv_hysteresis_init(&hyst, 100, 20, false);
	TEST_ASSERT_FALSE(uv_hysteresis_step(&hyst, 50));
	TEST_ASSERT_TRUE(uv_hysteresis_step(&hyst, 150));
	TEST_ASSERT_EQ(uv_hysteresis_get_output(&hyst), 1);
}
```

Available assertions are in `uv_test.h`: `TEST_ASSERT_TRUE`, `TEST_ASSERT_FALSE`,
`TEST_ASSERT_EQ`, `TEST_ASSERT_NE`, `TEST_ASSERT_NEAR`, `TEST_ASSERT_RANGE`,
`TEST_ASSERT_STR_EQ`, `TEST_ASSERT_NULL`, `TEST_ASSERT_NOT_NULL`, `TEST_FAIL`.

To bring a new uv_hal module under test, add its `.c` to `HAL_SOURCES` in the
makefile and enable whatever `CONFIG_*` it needs in `config/uv_hal_config.h`.

### The framework is shared

`uv_test.h` and `uv_test.c` are also used by the **uvcan** submodule's suite
(`uvcan/tests/`), which builds them straight out of this directory rather than
keeping a copy. uvcan cannot be built without uv_hal anyway, so this costs
nothing and keeps the two suites in step. Bear it in mind when changing the
framework: run both suites afterwards. A suite names itself in the runner banner
through `-DUV_TEST_SUITE_NAME`.

### `TEST_XFAIL` — tests for known defects

`TEST_XFAIL(suite, name)` declares a test that is *expected to fail*. Use it to
record a defect in executable form: the test asserts what the code **should** do,
and the runner reports `xfail` without failing the build.

This avoids the two bad alternatives — freezing buggy behaviour into an assertion
(which makes the eventual fix look like a regression) and leaving a red suite
that everyone learns to ignore.

If the defect gets fixed, the runner reports `xpass` and fails, which forces the
test to be promoted to a plain `TEST()` so it guards the fix from then on. Always
explain the defect in a comment above the test.

## What is covered

| Module | What the tests pin down |
|---|---|
| `uv_filters.c` | moving average, EWMA and hysteresis: settling accuracy, convergence, no chatter around the threshold |
| `uv_pid.c` | fixed point P/I/D scaling, step-time normalisation, integrator windup clamps, enable/disable |
| `uv_utilities.c` | `uv_delay`, ring buffer, vector, and the integer maths helpers (`lerpi`, `reli`, `ctz`, `isqrt`, …) |
| `uv_json.c` | writer output format and buffer overflow handling, reader traversal, arrays, round trip |
| `canopen_sdo.c`, `canopen_sdo_server.c`, `canopen_obj_dict.c` | the SDO wire protocol — see below |

### CANopen SDO

SDO is how every parameter on every device is read and written: by the `uvcan`
command line tool, by the uv0d display over the bus, and by the bootloader during
a firmware update. It is a wire protocol, so the tests are written against the
wire — each hands the stack the exact bytes a real master would send and asserts
on the exact bytes that come back.

That is deliberate. Asserting on frames rather than on internal state means the
tests survive a restructuring of the implementation, and it means a change that
would break interoperability with an existing master — the thing that actually
costs a service visit — fails here first.

Covered: expedited read and write of 8/16/32 bit objects; read-only, write-only
and missing objects; array element access and bounds; node id range checking;
segmented upload and download of strings and arrays including the toggle bit,
the final-segment length encoding, master aborts and protocol timeouts; COB-ID
addressing and frame filtering; the standard identity and node id objects; and
the read/write callbacks.

The seam is `stubs/canopen_stubs.c`, which replaces `uv_can_send()` with a
capture buffer and supplies a small object dictionary with one entry per case the
server has to handle. `uv_canopen.c` itself is *not* linked — it drags in the
NMT, PDO, heartbeat and EMCY modules and the CAN hardware behind them, none of
which the SDO protocol needs.

Not covered: the SDO **client** (`canopen_sdo_client.c` is linked but has no
tests of its own yet — it is the obvious next step, and the capture buffer is
already the right seam for it), and block transfer, which is disabled in this
build as it is on the devices.

## What is deliberately **not** covered

Only modules with no hardware dependency are here. That is not a coverage
target, it is the boundary that keeps these tests honest: a test whose result
depends on a stub of an ADC or a display controller mostly tests the stub.

- **Peripheral drivers** (`uv_adc`, `uv_can`, `uv_spi`, `uv_i2c`, `uv_ft81x`,
  `uv_w25q128`, …) — these are the hardware, and belong on a bench or in the
  simulator.
- **Output modules** (`uv_prop_output`, `uv_solenoid_output`,
  `uv_dual_solenoid_output`, `uv_ref_output`). These hold real logic worth
  testing — dither, ramping, current scaling — but their headers pull in
  `uv_adc_channels_e` and `uv_gpios_e`, which need target specific ADC
  configuration. Bringing them in means adding a `pin_mappings.h` stub to
  `config/`; worth doing, not done here.
- **CANopen stack**, **RTOS integration**, **UI framework** — these need a bus,
  a scheduler or a display respectively.
- **Anything timing, stack depth or RAM related.** Host tests cannot see stack
  overflow, ISR latency or heap exhaustion. Those still need the simulator and
  real hardware. The point of the tests here is to take the *logic* bugs off the
  bench so that the bench time goes to the bugs that genuinely need it.

## Defects these tests found, and now guard

Writing the suite turned up ten defects in uv_hal. All are fixed; the tests that
found them are ordinary `TEST()` cases now, each with a comment explaining what
the code used to do so that nobody reintroduces it.

| Test now guarding it | Defect |
|---|---|
| `json_reader.init_does_not_write_past_the_end_of_the_buffer` | `json_remove_whitespace()` wrote the terminating NUL at `buffer[buffer_len]` when the JSON contained no strippable whitespace — a one byte out of bounds **write** into the caller's buffer, on a target with no MMU. Machine generated JSON, including everything `uv_jsonwriter` produces, hits this every time |
| `sdo_segmented_read.the_final_segment_reports_its_length_in_bits_3_to_1` | the SDO server encoded the final segment's unused-byte count (`n`) in bits 2..0 instead of bits 3..1, overlapping the "no more segments" flag. Every segmented upload whose last frame did not carry exactly 7 bytes announced the wrong length. Our own client is shielded by the total size from the initiate reply; a spec-compliant master reads trailing garbage |
| `vector.push_front_prepends_and_grows_the_vector` | `uv_vector_push_front()` did `len--` instead of `len++` and shifted only one element. On an empty vector the length wrapped to 65535 |
| `vector.pop_front_shifts_the_remaining_elements_down` | `uv_vector_pop_front()` shifted one element instead of `len - 1`, silently losing and duplicating elements |
| `vector.remove_does_not_read_past_the_end_of_the_buffer` | `uv_vector_remove()` moved one element too many, reading past the live data and, when the vector was full, past the buffer |
| `ring_buffer.pop_front_removes_the_newest_element` | `uv_ring_buffer_pop_front()` read the never written slot at `head` and advanced `head` forwards, returning garbage and desynchronising the buffer. `uvcan/hhead_dia.c` uses it for reverse feeding, so this corrupted the diameter profile bucking decisions are made from |
| `pid.stays_disabled_once_it_has_reached_zero` | `uv_pid_step()` calls `uv_pid_reset()` when shutting down, and `uv_pid_reset()` set `state = PID_STATE_ON` — so a disabled PID re-enabled itself on the very step meant to stop it |
| `pid.reset_does_not_re_enable_a_disabled_controller` | same root cause: `uv_pid_reset()` conflated clearing history with changing the enable state. It now only clears history |
| `json_reader.get_child_on_an_empty_object_returns_null` | `uv_jsonreader_get_child()` returned a pointer to the closing `}` of an empty object, so every empty object looked like it had one member |
| `moving_aver.init_rejects_zero_count_like_set_count_does` | `uv_moving_aver_init()` accepted a count of 0 while `uv_moving_aver_set_count()` substituted 1 — the two entry points disagreed |

`make san` also reported two pieces of undefined behaviour in `uv_utilities.c`,
both fixed. Neither produced a wrong answer on arm-none-eabi-gcc, but both were
UB an optimiser is entitled to break, and both were MISRA violations:

- `uv_ctz()` negated `INT32_MIN` when called with `0x80000000`; the negation is
  now done in unsigned arithmetic.
- `uv_countofbit()` evaluated `1 << 31` on a signed `int`; the constant is now
  unsigned.
