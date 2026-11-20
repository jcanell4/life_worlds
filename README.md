([Llegeix en català](LLEGEIXME.md))
([Leer en español](LEEME.md))

# life_worlds

ECS simulation in C++ with [EnTT](https://github.com/skypjack/entt) and visualization with [raylib](https://www.raylib.com/): an N×N world with configurable modules (`random_plant`, `energy_storage_plant`, `ql_agent`, `c51_agent`, `dqn_agent`) decoupled from the grid via **ports** and **adapters**, and drawn with **render layers** registered in the composition root.

## Requirements

- CMake ≥ 3.20
- C++17 compiler (Apple Clang, Clang, or GCC)
- raylib 5.x
- [LibTorch](https://pytorch.org/get-started/locally/) (CPU is enough) for the C51 agent

macOS (Homebrew, Apple Silicon or Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). The system package avoids building raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

If raylib is not installed, CMake downloads it with FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` disables that). If LibTorch is missing, `LIFE_WORLDS_FETCH_LIBTORCH=ON` (default) downloads the official CPU zip once under `~/.local/libtorch-cpu/libtorch`; on network failure the error explains how to point CMake at a Python Torch install (`python3 -c "import torch; print(torch.utils.cmake_prefix_path)"`). EnTT and [nlohmann/json](https://github.com/nlohmann/json) are always fetched the first time (Internet required). To build raylib from source on Linux you also need window and OpenGL headers:

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
# Point CMAKE_PREFIX_PATH at a LibTorch install (prebuilt zip or CI cache).
export CMAKE_PREFIX_PATH="$HOME/.local/libtorch-cpu/libtorch${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

At runtime, add LibTorch's `lib/` to `LD_LIBRARY_PATH` (Linux) or `DYLD_LIBRARY_PATH` (macOS) if the dynamic linker does not find `libtorch.so`.

CI: GitHub Actions (`.github/workflows/build.yml`) builds on Linux, macOS, and Windows (raylib/EnTT/json via FetchContent + LibTorch CPU), runs `--console` smokes for QL and C51, and uploads **artifacts** (`life_worlds-linux` / `-macos` / `-windows`) with the binary and the example configs. Download them under **Actions** → run → Artifacts (retained 14 days).

## Vision, observation, and stop criteria

### Vision configuration

- `vision_rays` (on each agent entity: `ql_agent` / `c51_agent` / `dqn_agent`): `4` (default) or `8`. Different agent entities in the same run may mix 4 and 8.
  - **4** — orthogonal: N, S, W, E (historical order).
  - **8** — octants: N, NE, E, SE, S, SW, W, NW.
- If `environment.vision_rays` is still present, compose fails with a clear error (move it to the agent).
- Movement actions stay the 4 cardinals only; Eat/Cut only on orthogonal neighbours.
- `max_vision` (on the agent): ray length cap in cells (`0` = `grid_size`).

Each ray reports the **first** hit: a `random_plant` (`herb` / `shrub`), another agent, or edge/empty (`empty_edge`). It also fills `growth_stage` (see below).

### Shared buckets (all agents)

| Field | Values |
|-------|--------|
| Distance (cells) | `0` = 1; `1` = 2–4; `2` = 5–10; `3` = >10 |
| Type | `herb=0`, `shrub=1`, `agent=2`, `empty_edge=3` |
| Energy | `3` = [75%,100%]; `2` = [50%,75%); `1` = [25%,50%); `0` = [0%,25%) |
| `growth_stage` | float in `[0,1]`: `0` if no plant; else `(stage_index+1)/stage_count` |

Changing a species' stage count does **not** change the neural vector size (only the scalar value).

### QL (tabular state)

Ignores `growth_stage`. Encodes `ray_count` digits `(distance, type)` + energy bucket.

- With `vision_rays: 4`: \((4 \times 4)^4 \times 4 = 262\,144\) theoretical states.
- With `vision_rays: 8`: \((4 \times 4)^8 \times 4 = 17\,179\,869\,184\) theoretical states (sparse table; many more keys appear).

### C51 / DQN (feature vector)

Per ray, in order: distance one-hot (4) + type one-hot (4) + `growth_stage` (1). Then energy one-hot (4).

| `vision_rays` | Feature size |
|---------------|--------------|
| 4 | \(4 \times (4+4+1) + 4 = 40\) |
| 8 | \(8 \times (4+4+1) + 4 = 76\) |

`.pt` models trained with 4 rays are **not** compatible with 8 (and vice versa). They stay compatible if only plant stage counts change.

### Precipice and training stop

- `precipice_reward` (agent, default `-20`): leaving the grid ends the episode.
- `survive_cycles` (root, `0` = off): stop when every active agent has `EpisodeCycles >= survive_cycles`. Only applies if at least one agent process is in `train`; ignored in pure `eval`.

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
./build/linux/life_worlds --config config/random_plant_ql_1.json --console
```

In the window: empty cells are light green; each species is drawn with a palette color by index (circle if the index is even, square if odd) and QL agents are orange triangles, unless the config sets a [`visual`](#look-of-plants-and-agents-visual) for them. The bottom bar shows tick and counters. **F8** or **Ctrl+P** toggles fast mode (skips drawing the world).

## Architecture (ports and plugins)

- **Ports** (`ports/`): grid-free contracts — `OccupancyPort` (`free_cells`) for plants; `AgentWorldPort` (`observe`, `apply_action`, spawn/reset) for QL and C51.
- **Grid adapters** (`environment/grid/adapters`): implement the ports by delegating to `GridEnvironment`.
- **Render layers** (`environment/grid/render_layers`): the ways of drawing each kind of environment offers, registered with `Environment::add_render_layer`. The grid ones draw in cells with shared steps (`CellCanvas`: shape, label) and read only `core` components (`Position`, `Visual`, `Energy`), so they do not depend on any module: each entity picks its own with [`visual.render_layer`](#look-of-plants-and-agents-visual). Another kind of environment would define its own set.
- **Composition root** (`app/grid_compose`): `compose_grid_app` creates the grid, wires ports + layers, and builds each `Entity` with injected ports. Plant/QL processes **do not** know about `GridEnvironment`.
- **Events** (`core/events`): modules report what happens by emitting a typed `events::Event` (birth, death, eat, cut…) where it happens; the simulation tallies births and deaths once per tick from those events.

## Configuration (JSON)

Edit [`config/random_plant_ql_1.json`](config/random_plant_ql_1.json). `load_config` parses the simulation settings and calls `compose_grid_app`, which creates the `environment`, registers draw layers, and one `Entity` per `entities` entry (order = `step` order).

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
| `survive_cycles` | Stop when every active agent has survived this many cycles (`0` = off; ignored if every agent is `eval`) |
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

### Look of plants and agents (`visual`)

Each plant species (`random_plant`, `energy_storage_plant`) and each agent entity (`ql_agent`, `c51_agent`, `dqn_agent`) accepts an optional compound attribute `visual`. All three fields are optional; whatever is missing keeps its module's default.

```json
"visual": { "shape": "circle", "color": [72, 168, 74], "render_layer": "plain" }
```

| Field | Values | Default |
|-------|--------|---------|
| `shape` | `"circle"`, `"square"`, `"triangle"`, `"diamond"`, `"hexagon"` | `random_plant`: circle if the species index is even, square if odd; `energy_storage_plant`: hexagon; agents: triangle |
| `color` | `[r, g, b]` or `[r, g, b, a]` (0-255), or a string `"#RRGGBB"` / `"#RRGGBBAA"` | `random_plant`: palette by species index; `energy_storage_plant`: one color per lineage; `ql_agent`: orange; `c51_agent`: blue |
| `render_layer` | `"plain"`, `"level"`, `"value_below"` | `random_plant`: `plain`; `energy_storage_plant`: `level`; agents: `value_below` |

`render_layer` picks the grid render layer that draws the entity. Layers do not depend on the module, so any module can use any of them:

| Layer | What it draws |
|-------|---------------|
| `plain` | The shape in its color |
| `level` | The shape, brighter the fuller its energy is (value / maximum), with the value in the centre |
| `value_below` | The shape in its color with its energy value underneath |

An entity without energy (`random_plant` plants) looks the same with all three. Draw order is fixed: empty cells, residues, `plain`, `level`, `value_below`. An unknown layer name is a configuration error.

Notes: a plant eaten or cut leaves a residue with the **shape** of its species (gray = eaten, brown = cut, whatever the species color); residues have their own layer, which is not selectable. On `energy_storage_plant`, setting `color` makes every lineage of that species share it.

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
| `plant_types[].visual` | Optional `{ shape, color, render_layer }`, see [Look of plants and agents](#look-of-plants-and-agents-visual) |

### Entity `energy_storage_plant`

Plants that accumulate energy every cycle and split into an adjacent free cell when full. Example config: [`config/energy_storage_plant_1.json`](config/energy_storage_plant_1.json) (`--config config/energy_storage_plant_1.json`).

- Each cycle a plant gains `growing_rate` energy, up to `maximum_energy_limit`. With no free orthogonal neighbour cell it does not grow or accumulate energy (it resumes if a neighbour frees up).
- At maximum energy it creates a new plant (same species) in a free adjacent cell: the child receives `split_fraction` of the parent's energy and the parent keeps the rest.
- **Lineage**: every initial plant starts a lineage and its descendants share it (same colour in the window; with the default layer, `level`, brightness follows stored energy and each plant shows its value; hexagons by default, configurable with `visual`). A lineage cannot occupy more than `maximum_cell_occupancy` cells: once reached, its plants keep filling up to the energy maximum and then stop growing.
- Several species are defined like `random_plant`, each with its `name`. No spontaneous death.
- **Phase 1:** it does not interact with other entities. `random_plant` and `ql_agent` ignore these plants, so do not combine them in the same config yet (they could overlap in a cell).

| Parameter | Description |
|-----------|-------------|
| `type` | Must be `"energy_storage_plant"` |
| `plant_types[].name` | Species name |
| `plant_types[].initial_count` | Initial plants (each one a new lineage) |
| `plant_types[].initial_energy` | Starting energy of each initial plant (`0` ≤ value ≤ `maximum_energy_limit`) |
| `plant_types[].growing_rate` | Energy added per cycle (`>= 0`) |
| `plant_types[].maximum_energy_limit` | Energy cap of every plant (`> 0`) |
| `plant_types[].maximum_cell_occupancy` | Max cells per lineage (`>= 1`) |
| `plant_types[].split_fraction` | Fraction of the parent's energy given to the new plant (`0 < value < 1`, default `0.5`) |
| `plant_types[].visual` | Optional `{ shape, color, render_layer }` |

### Entity `ql_agent` (tabular Q-learning)

Place it **after** `random_plant` in `entities`. The agent **chooses** the action via `AgentWorldPort`; the grid adapter **applies** the effect on `GridEnvironment` (move/eat/cut). Actions: Up/Down/Left/Right/Eat/Cut. Observation (rays, buckets, state space by `vision_rays`) is described under [Vision, observation, and stop criteria](#vision-observation-and-stop-criteria). `max_vision` `0` uses `port.default_vision_range()` (on the grid: `grid_size`).

Movement: you cannot enter a cell with a plant or another agent (you must **eat** or **cut** first). Eat/Cut only act on an **adjacent** plant (Up/Down/Left/Right order if more than one); not from the same cell.

After Eat/Cut a residue remains in the cell (**gray** = eaten, **brown** = cut) until a living plant is born there. The HUD/log shows cumulative counters `eat_<species>` and `cut_<species>`.

| Parameter | Description |
|-----------|-------------|
| `type` | `"ql_agent"` |
| `mode` | `"train"` (ε-greedy + Q update; energy 0 → reset) or `"eval"` (greedy; energy 0 → score and inactive; on exit it prints the score — cycles survived — of every agent, and the energy left for those still alive) |
| `agent_count` | Number of agents to spawn |
| `energy_max` / `energy_drain` | Max energy and per-cycle drain (default 100 / 5) |
| `herb_energy_delta` / `shrub_energy_delta` | Energy change (and eat reward) on Eat |
| `ingestion_factor` | Multipliers `[herb, shrub]` on the energy delta (default `[1,1]`) |
| `alpha` / `gamma` / `epsilon` | Q-learning hyperparameters |
| `precipice_reward` | Reward when leaving the grid (default −20) |
| `vision_rays` | `4` (orthogonal) or `8` (octants); shared by every agent of this process (default 4) |
| `max_vision` | Distance cap (`0` = `grid_size`) |
| `q_table_path` | Binary Q-table file (saved on train shutdown; loaded in eval or if `reuse_q_table`) |
| `reuse_q_table` | In `train`, if `true` loads the existing table and continues learning; if `false` starts from scratch |
| `visual` | Optional `{ shape, color, render_layer }` for every agent of this entity |

Per-cycle rewards (energy band, after drain/eat): `>75%` → −1; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. The eat reward is the energy delta of the Eat action.

### Entity `c51_agent` (Categorical DQN / C51)

Same world interface as `ql_agent` (same `AgentWorldPort`, actions, rewards, train/eval lifecycle). Instead of a sparse Q-table it learns a distribution over returns with a small MLP (LibTorch), shared by every agent of the process. The feature vector (40 or 76 floats depending on `vision_rays`, with `growth_stage`) is described under [Vision, observation, and stop criteria](#vision-observation-and-stop-criteria). Example config: [`config/random_plant_c51_1.json`](config/random_plant_c51_1.json) (same plants as `random_plant_1`).

| Parameter | Description |
|-----------|-------------|
| `type` | `"c51_agent"` |
| `mode` | `"train"` (ε-greedy + replay updates; energy 0 → reset) or `"eval"` (greedy; energy 0 → inactive) |
| `agent_count` / `energy_max` / `energy_drain` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` | Same meaning as `ql_agent` |
| `n_atoms` | Atoms of the return distribution (default 51) |
| `v_min` / `v_max` | Support of the distribution (default −20 / 20) |
| `hidden_sizes` | MLP widths (default `[128, 128]`) |
| `learning_rate` / `batch_size` / `replay_capacity` | Adam + replay |
| `learn_start` | Transitions before the first update |
| `train_every` / `target_update_every` | Update cadence and hard target sync |
| `model_path` | Torch archive (saved on train shutdown; loaded in eval or if `reuse_model`) |
| `reuse_model` | In `train`, continue from an existing file |
| `visual` | Optional `{ shape, color, render_layer }` (default blue triangle) |


### Entity `dqn_agent` (Double DQN)

Same world interface as `ql_agent` / `c51_agent`. Learns Q-values with Double DQN (LibTorch), shared network and replay. Features: same as C51 (see [Vision, observation, and stop criteria](#vision-observation-and-stop-criteria)). Example config: [`config/random_plant_dqn_1.json`](config/random_plant_dqn_1.json).

| Parameter | Description |
|-----------|-------------|
| `type` | `"dqn_agent"` |
| `mode` | `"train"` or `"eval"` (same reset/inactive lifecycle as C51) |
| `agent_count` / `energy_*` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` / `precipice_reward` | As `ql_agent` / `c51_agent` |
| `hidden_sizes` / `learning_rate` / `batch_size` / `replay_capacity` | MLP + replay |
| `learn_start` / `train_every` / `target_update_every` | Learning cadence |
| `model_path` / `reuse_model` | `.pt` checkpoint |
| `visual` | `{ shape, color, render_layer }` optional (green triangle by default) |

## Structure

- `ports/` — `OccupancyPort`, `AgentWorldPort` (grid-free contracts)
- `core/` — shared components (`Position`, `Age`, `Energy`, `Visual`), `StepContext`, tick metrics, `events/` (typed events shared by all modules + birth/death tally)
- `entity/` — `Entity` interface
- `environment/` — `Environment` interface (`add_render_layer`) + `grid/` (queries, `apply_action`, `adapters/`, `render_layers/`: one layer per file + `CellCanvas`)
- `plant/components.hpp`, `plant/common.hpp` — component (`PlantTypeIndex`) and helpers (alive counts) shared by the plant modules
- `plant/random_plant/` — population dynamics + visual traits (occupancy via port), residues of eaten/cut plants (`residue`)
- `plant/energy_storage_plant/` — energy-storing plants with lineages and a per-lineage cell cap
- `agent/` — shared `Agent` / `EpisodeCycles` / `Inactive` and vision types; `ql_agent/` (tabular Q-learning); `c51_agent/` (C51 / LibTorch); `dqn_agent/` (Double DQN)
- `app/` — grid composition root (`compose_grid_app`: ports + layers + entities)
- `simulation/` — orchestration, display config
- `config_loader.cpp` — simulation JSON + call to `compose_grid_app`
- `main.cpp` — `--config`, `--console` / visual by default
