([Llegeix en català](LLEGEIXME.md))
([Leer en español](LEEME.md))

# life_worlds

ECS simulation in C++ with [EnTT](https://github.com/skypjack/entt) and visualization with [raylib](https://www.raylib.com/): an N×N world with configurable agents (currently `random_plant` plants) that are born, age through stages, and die.

## Requirements

- CMake ≥ 3.20
- C++17 compiler (Apple Clang, Clang, or GCC)
- raylib 5.x

macOS (Homebrew, Apple Silicon or Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). The system package avoids building raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

If raylib is not installed, CMake downloads it with FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` disables that). EnTT and [nlohmann/json](https://github.com/nlohmann/json) are always fetched the first time (Internet required). To build raylib from source on Linux you also need window and OpenGL headers:

```bash
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev libxkbcommon-dev libwayland-dev libasound2-dev
```

The build directory depends on the OS (`build/darwin` on macOS, `build/linux` on Linux) so the two machines do not share the same CMake cache. Editor builds use Debug.

## Build

Clean rebuild on the current machine (deletes only `build/darwin` or `build/linux`):

```bash
./rebuild.sh
```

In the editor, the default **CMake: build** task configures and builds (on macOS: ⇧⌘B). Debugging or running with a `launch.json` configuration builds first when needed.

From the terminal, use the visible system preset (CMake ≥ 3.21):

```bash
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

## Run

Run from the project root (or pass a valid path to `--config`) so the JSON can be found.

Visual mode (raylib window; Esc or close to stop):

```bash
./build/darwin/life_worlds   # macOS
./build/linux/life_worlds    # Linux
```

Console / headless mode (no window):

```bash
./build/darwin/life_worlds --console
# or:
LIFE_WORLDS_HEADLESS=1 ./build/linux/life_worlds
```

Alternate config:

```bash
./build/linux/life_worlds --config config/default.json --console
```

In the window: empty cells are light green; each species is drawn with a palette color by index (circle if the index is even, square if odd); QL agents are orange triangles. The bottom bar shows tick and counters. **F8**, **F12**, or **Ctrl+P** toggles fast mode (skips drawing the world; prefer **F8**, because raylib uses F12 for screenshots).

## Configuration (JSON)

Edit [`config/default.json`](config/default.json). `Simulation::init` loads the file, creates the `environment`, and one `Entity` per `entities` entry (order = `step` order).

### Simulation (root)

| Parameter | Description |
|-----------|-------------|
| `cell_size_px` | Pixels per cell in the window |
| `hud_height_px` | Height in pixels of the bottom bar |
| `hud_lines` | Number of HUD rows (≥ 1); metrics are spread across them |
| `ticks_per_second` | Simulation tick rate with the world visible (independent of FPS) |
| `display_fps` | Target FPS while drawing the world (default 60) |
| `fast_display_fps` | How often the overlay is redrawn in fast mode (does not throttle steps; default 30) |
| `fast_slice_seconds` | How often the keyboard is polled in fast mode while steps run (default ~1/60 s) |
| `tick_count` | Tick limit (`0` = no limit in visual; 40 in console) |
| `rng_seed` | Random generator seed |
| `log_tick_summary` | One summary line per tick on the console |
| `log_events` | Detailed typed-event logging |
| `cycles_per_time_unit` | Cycles per time unit (e.g. one year) |
| `environment` | World (`type: grid`, `grid_size`, …) |
| `entities` | List of modules/`Entity` to run each tick |

### Environment `grid`

| Parameter | Description |
|-----------|-------------|
| `type` | `"grid"` |
| `grid_size` | N of the N×N grid |

### Entity `random_plant`

| Parameter | Description |
|-----------|-------------|
| `type` | Must be `"random_plant"` |
| `offspring_inheritance_jitter` | Maximum random variation (±) when inheriting fertility (`0` = exact copy) |
| `plant_types[].name` | Species name |
| `plant_types[].min_population` | Minimum individuals; `0` = no forced seeding |
| `plant_types[].max_population` | Maximum individuals (reproduction and seeding do not exceed it); `0` = no limit |
| `plant_types[].stage_count` | Number of stages (max 8) |
| `plant_types[].time_units_by_stage` | Duration of each stage in time units (`<= 0` → 1) |
| `plant_types[].death_rate_by_stage` | Death probability per stage |
| `plant_types[].offspring_rate_by_stage` | Offspring probability per stage (initial value) |

### Entity `ql_agent` (tabular Q-learning)

Place it **after** `random_plant` in `entities`. The agent **chooses** the action; `GridEnvironment` **applies** the effect (move/eat/cut). Actions: Up/Down/Left/Right/Eat/Cut. State: 4-direction vision `(distance_bucket, type)` with types `herb=0`, `shrub=1`, `ql_agent=2`, `empty_edge=3` and distance `0` (1 cell), `1` (2–4), `2` (5–10), `3` (>10), plus energy level `3` ([75%,100%]), `2` ([50%,75%)), `1` ([25%,50%)), `0` ([0%,25%)). Theoretical space: \((4 \times 4)^4 \times 4 = 262144\) states.

Movement: you cannot enter a cell with a plant or another QL agent (you must **eat** or **cut** first). Eat/Cut only act on an **adjacent** plant (Up/Down/Left/Right order if more than one); not from the same cell.

After Eat/Cut a residue remains in the cell (**gray** = eaten, **brown** = cut) until a living plant is born there. The HUD/log shows cumulative counters `eat_<species>` and `cut_<species>`.

| Parameter | Description |
|-----------|-------------|
| `type` | `"ql_agent"` |
| `mode` | `"train"` (ε-greedy + Q update; energy 0 → reset) or `"eval"` (greedy; energy 0 → score and inactive) |
| `agent_count` | Number of agents to spawn |
| `energy_max` / `energy_drain` | Max energy and per-cycle drain (default 100 / 5) |
| `herb_energy_delta` / `shrub_energy_delta` | Energy change (and eat reward) on Eat |
| `ingestion_factor` | Multipliers `[herb, shrub]` on the energy delta (default `[1,1]`) |
| `alpha` / `gamma` / `epsilon` | Q-learning hyperparameters |
| `max_vision` | Distance cap (`0` = `grid_size`) |
| `q_table_path` | Binary Q-table file (saved on train shutdown; loaded in eval or if `reuse_q_table`) |
| `reuse_q_table` | In `train`, if `true` loads the existing table and continues learning; if `false` starts from scratch |

Per-cycle rewards (energy band, after drain/eat): `>75%` → +2; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. The eat reward is the energy delta of the Eat action.

## Structure

- `core/` — `Position`/`Age`, `StepContext`, tick metrics
- `entity/` — `Entity` interface
- `environment/` — `Environment` interface + `grid/` (queries, `apply_action`, render)
- `plant/random_plant/` — population dynamics + visual traits
- `agent/ql_agent/` — Q-learning (chooses actions; learns from outcomes)
- `simulation/` — orchestration, display config, `events/`
- `config_loader.cpp` — JSON (`environment` + `entities`)
- `main.cpp` — `--config`, `--console` / visual by default
