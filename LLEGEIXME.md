([Leer en español](LEEME.md))
([Read in English](README.md))

# life_worlds

Simulació ECS en C++ amb [EnTT](https://github.com/skypjack/entt) i visualització amb [raylib](https://www.raylib.com/): un món N×N amb mòduls configurables (`random_plant`, `energy_storage_plant`, `ql_agent`) desacoblats del grid mitjançant **ports** i **adapters**, i dibuixats amb **capes de render** registrades al composition root.

## Requisits

- CMake ≥ 3.20
- Compilador C++17 (Apple Clang, Clang o GCC)
- raylib 5.x

macOS (Homebrew, Apple Silicon o Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). El paquet del sistema evita compilar raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

Si raylib no està instal·lat, CMake el descarrega amb FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` ho desactiva). EnTT i [nlohmann/json](https://github.com/nlohmann/json) sempre es descarreguen la primera vegada (cal Internet). Per compilar raylib des del codi font a Linux també calen les capçaleres de finestra i OpenGL:

```bash
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev libxkbcommon-dev libwayland-dev libasound2-dev
```

El directori de build depèn del sistema (`build/darwin` a macOS, `build/linux` a Linux) perquè les dues màquines no comparteixin la mateixa cache de CMake. La compilació des de l’editor és en Debug.

## Compilar

Recompilació neta a la màquina actual (esborra només `build/darwin` o `build/linux`):

```bash
./rebuild.sh
```

A l’editor, la tasca per defecte **CMake: build** configura i compila (a macOS: ⇧⌘B). Depurar o executar amb una configuració de `launch.json` compila abans, si cal.

Des de la terminal, el preset visible és el del sistema (CMake ≥ 3.21):

```bash
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

CI: a GitHub Actions (`.github/workflows/build.yml`) compila en Linux, macOS i Windows (raylib/EnTT/json via FetchContent), fa un smoke `--console` i puja **artifacts** (`life_worlds-linux` / `-macos` / `-windows`) amb el binari i `config/random_plant_1.json`. Descarrega’ls a **Actions** → run → Artifacts (caduquen als 14 dies).

## Executar

Cal executar des de l’arrel del projecte (o passar una ruta vàlida a `--config`) perquè es trobi el JSON.

Mode visual (finestra raylib; Esc o tancar per aturar):

```bash
./build/darwin/life_worlds   # macOS
./build/linux/life_worlds    # Linux
```

Mode consola / headless (sense finestra):

```bash
./build/darwin/life_worlds --console
# o:
LIFE_WORLDS_HEADLESS=1 ./build/linux/life_worlds
```

Configuració alternativa:

```bash
./build/linux/life_worlds --config config/random_plant_1.json --console
```

A la finestra: cel·les buides en verd clar; cada espècie es dibuixa amb color de paleta segons l’índex (cercle si l’índex és parell, quadrat si és senar) i els agents QL són triangles taronja, tret que el config hi defineixi un [`visual`](#aspecte-de-plantes-i-agents-visual). La barra inferior mostra tick i comptadors. **F8** o **Ctrl+P** activa/desactiva el mode ràpid (sense dibuixar el món).

## Arquitectura (ports i plugins)

- **Ports** (`ports/`): contractes sense grid — `OccupancyPort` (`free_cells`) per a plantes; `AgentWorldPort` (`observe`, `apply_action`, spawn/reset) per a QL.
- **Adapters grid** (`environment/grid/adapters`): implementen els ports delegant a `GridEnvironment`.
- **Render layers** (`environment/grid/layers`): plugins visuals (`draw_grid`, residus, plantes, agents QL) registrats amb `Environment::add_render_layer`.
- **Composition root** (`app/grid_compose`): `compose_grid_app` crea el grid, cables ports + capes, i construeix les `Entity` amb els ports injectats. Els process de plant/QL **no** coneixen `GridEnvironment`.
- **Events** (`core/events`): els mòduls informen del que passa emetent un `events::Event` tipat (naixement, mort, eat, cut…) allà on passa; la simulació compta naixements i morts un sol cop per tick a partir d’aquests esdeveniments.

## Configuració (JSON)

Edita [`config/random_plant_1.json`](config/random_plant_1.json). `load_config` parseja la simulació i crida `compose_grid_app`, que crea l’`environment`, registra les capes de dibuix i una `Entity` per cada entrada de `entities` (ordre = ordre de `step`).

### Simulació (arrel)

| Paràmetre | Descripció |
|-----------|------------|
| `cell_size_px` | Píxels per cel·la a la finestra |
| `hud_height_px` | Alçada en píxels de la barra inferior |
| `hud_lines` | Nombre de files del HUD (≥ 1); les mètriques es reparteixen entre elles |
| `ticks_per_second` | Ritme de ticks de simulació amb el món visible (independent del FPS) |
| `display_fps` | FPS objectiu mentre es dibuixa el món (defecte 60) |
| `fast_display_fps` | Cada quan es redibuixa l’overlay en mode ràpid (sense limitar els steps; defecte 30) |
| `fast_slice_seconds` | Cada quan es comprova el teclat en mode ràpid mentre s’executen steps (defecte ~1/60 s) |
| `tick_count` | Límit de ticks (`0` = sense límit en visual; 40 en consola) |
| `rng_seed` | Llavor del generador aleatori |
| `log_tick_summary` | Una línia de resum per tick a la consola |
| `log_events` | Log detallat d’esdeveniments tipats |
| `cycles_per_time_unit` | Cicles per unitat de temps (p. ex. un any) |
| `environment` | Món (`type: grid`, `grid_size`, …) |
| `entities` | Llista de mòduls/`Entity` a executar cada tick |

### Environment `grid`

| Paràmetre | Descripció |
|-----------|------------|
| `type` | `"grid"` |
| `grid_size` | N de la graella N×N |

### Aspecte de plantes i agents (`visual`)

Cada espècie de planta (`random_plant`, `energy_storage_plant`) i cada entitat `ql_agent` admet un atribut compost opcional `visual`. Els dos camps són opcionals; el que falti conserva l’aspecte per defecte.

```json
"visual": { "shape": "circle", "color": [72, 168, 74] }
```

| Camp | Valors | Per defecte |
|------|--------|-------------|
| `shape` | `"circle"`, `"square"`, `"triangle"`, `"diamond"`, `"hexagon"` | `random_plant`: cercle si l’índex de l’espècie és parell, quadrat si és senar; `energy_storage_plant`: hexàgon; `ql_agent`: triangle |
| `color` | `[r, g, b]` o `[r, g, b, a]` (0-255), o una cadena `"#RRGGBB"` / `"#RRGGBBAA"` | `random_plant`: paleta segons l’índex de l’espècie; `energy_storage_plant`: un color per filiació; `ql_agent`: taronja |

Notes: una planta menjada o tallada deixa un residu amb la **forma** de la seva espècie (gris = menjada, marró = tallada, sigui quin sigui el color de l’espècie). A `energy_storage_plant` la brillantor segueix sempre l’energia emmagatzemada; si es defineix `color`, totes les filiacions d’aquesta espècie el comparteixen.

### Entitat `random_plant`

| Paràmetre | Descripció |
|-----------|------------|
| `type` | Ha de ser `"random_plant"` |
| `offspring_inheritance_jitter` | Variació aleatòria màxima (±) en heretar fertilitat (`0` = còpia exacta) |
| `plant_types[].name` | Nom de l’espècie |
| `plant_types[].min_population` | Mínim d’exemplars; `0` = sense sembra forçada |
| `plant_types[].max_population` | Màxim d’exemplars (reproducció i sembra no el sobrepassen); `0` = sense límit |
| `plant_types[].stage_count` | Nombre d’etapes (màx. 8) |
| `plant_types[].time_units_by_stage` | Durada de cada etapa en unitats de temps (`<= 0` → 1) |
| `plant_types[].death_rate_by_stage` | Probabilitat de mort per etapa |
| `plant_types[].offspring_rate_by_stage` | Probabilitat de descendència per etapa (valor inicial) |
| `plant_types[].visual` | `{ shape, color }` opcional, vegeu [Aspecte de plantes i agents](#aspecte-de-plantes-i-agents-visual) |

### Entitat `energy_storage_plant`

Plantes que acumulen energia a cada cicle i es divideixen en una cel·la contigua lliure en omplir-se. Config d’exemple: [`config/energy_storage_plant_1.json`](config/energy_storage_plant_1.json) (`--config config/energy_storage_plant_1.json`).

- A cada cicle una planta guanya `growing_rate` d’energia, fins a `maximum_energy_limit`. Si no té cap cel·la veïna (ortogonal) lliure, no creix ni acumula energia (reprèn si una veïna s’allibera).
- En arribar a l’energia màxima crea una planta nova (mateixa espècie) en una cel·la contigua lliure: la filla rep `split_fraction` de l’energia de la mare i la mare conserva la resta.
- **Filiació**: cada planta inicial obre una filiació i els seus descendents la comparteixen (mateix color a la finestra; la brillantor segueix l’energia emmagatzemada; hexàgons per defecte, configurable amb `visual`). Una filiació no pot ocupar més de `maximum_cell_occupancy` cel·les: en assolir-ho, les seves plantes continuen omplint-se fins a l’energia màxima i llavors deixen de créixer.
- Es defineixen diverses espècies com a `random_plant`, cadascuna amb el seu `name`. No hi ha mort espontània.
- **Fase 1:** no interactua amb altres entitats. `random_plant` i `ql_agent` ignoren aquestes plantes, així que de moment no es combinen en el mateix config (podrien solapar-se en una cel·la).

| Paràmetre | Descripció |
|-----------|------------|
| `type` | Ha de ser `"energy_storage_plant"` |
| `plant_types[].name` | Nom de l’espècie |
| `plant_types[].initial_count` | Plantes inicials (cadascuna, una filiació nova) |
| `plant_types[].initial_energy` | Energia inicial de cada planta inicial (`0` ≤ valor ≤ `maximum_energy_limit`) |
| `plant_types[].growing_rate` | Energia que se suma per cicle (`>= 0`) |
| `plant_types[].maximum_energy_limit` | Topall d’energia de cada planta (`> 0`) |
| `plant_types[].maximum_cell_occupancy` | Màxim de cel·les per filiació (`>= 1`) |
| `plant_types[].split_fraction` | Fracció de l’energia de la mare que rep la planta nova (`0 < valor < 1`, per defecte `0.5`) |
| `plant_types[].visual` | `{ shape, color }` opcional |

### Entitat `ql_agent` (Q-learning tabular)

Cal posar-la **després** de `random_plant` a `entities`. L’agent **tria** l’acció via `AgentWorldPort`; l’adapter grid **aplica** l’efecte a `GridEnvironment` (move/eat/cut). Accions: Up/Down/Left/Right/Eat/Cut. Estat: visió en 4 direccions `(bucket_distància, tipus)` amb tipus `herb=0`, `shrub=1`, `ql_agent=2`, `empty_edge=3` i distància `0` (1 cel·la), `1` (2–4), `2` (5–10), `3` (>10), més el nivell d’energia `3` ([75%,100%]), `2` ([50%,75%)), `1` ([25%,50%)), `0` ([0%,25%)). Espai teòric: \((4 \times 4)^4 \times 4 = 262144\) estats. `max_vision` `0` usa `port.default_vision_range()` (al grid: `grid_size`).

Moviment: no es pot entrar a una cel·la amb planta ni amb un altre agent QL (cal **menjar** o **tallar** abans). Eat/Cut només actuen sobre una planta **colindant** (ordre Up/Down/Left/Right si n’hi ha més d’una); no des de la mateixa cel·la.

Després d’Eat/Cut queda un residu a la cel·la (**gris** = menjada, **marró** = tallada) fins que hi neix una planta viva. El HUD/log mostra comptadors acumulats `eat_<espècie>` i `cut_<espècie>`.

| Paràmetre | Descripció |
|-----------|------------|
| `type` | `"ql_agent"` |
| `mode` | `"train"` (ε-greedy + update Q; energia 0 → reset) o `"eval"` (greedy; energia 0 → score i inactiu; en sortir imprimeix el score —cicles aguantats— de tots els agents, i l’energia restant dels que segueixen vius) |
| `agent_count` | Nombre d’agents a generar |
| `energy_max` / `energy_drain` | Energia màxima i drenatge per cicle (defecte 100 / 5) |
| `herb_energy_delta` / `shrub_energy_delta` | Canvi d’energia (i reward de menjar) en Eat |
| `ingestion_factor` | Multiplicadors `[herb, shrub]` sobre el delta d’energia (defecte `[1,1]`) |
| `alpha` / `gamma` / `epsilon` | Hiperparàmetres Q-learning |
| `max_vision` | Tope de distància (`0` = `grid_size`) |
| `q_table_path` | Fitxer binari de la taula Q (save en train al tancar; load en eval o si `reuse_q_table`) |
| `reuse_q_table` | En `train`, si `true` carrega la taula existent i continua l’aprenentatge; si `false` comença de zero |
| `visual` | `{ shape, color }` opcional per a tots els agents d’aquesta entitat |

Recompenses per cicle (banda d’energia, després del drenatge/menjar): `>75%` → +2; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. El reward de menjar és el delta d’energia de l’acció Eat.

## Estructura

- `ports/` — `OccupancyPort`, `AgentWorldPort` (contractes sense grid)
- `core/` — components compartits (`Position`, `Age`, `Visual`), `StepContext`, mètriques de tick, `events/` (esdeveniments tipats comuns a tots els mòduls + recompte de naixements/morts)
- `entity/` — interfície `Entity`
- `environment/` — interfície `Environment` (`add_render_layer`) + `grid/` (queries, `apply_action`, `adapters/`, `layers/`)
- `plant/components.hpp`, `plant/common.hpp` — component (`PlantTypeIndex`) i helpers (recompte de vives) compartits pels mòduls de plantes
- `plant/random_plant/` — dinàmica poblacional + trets visuals (ocupació via port), residus de plantes menjades/tallades (`residue`)
- `plant/energy_storage_plant/` — plantes que emmagatzemen energia, amb filiacions i límit de cel·les per filiació
- `agent/ql_agent/` — Q-learning (tria accions; sent/actua via port)
- `app/` — composition root grid (`compose_grid_app`: ports + capes + entities)
- `simulation/` — orquestració, config de display
- `config_loader.cpp` — JSON de simulació + crida a `compose_grid_app`
- `main.cpp` — `--config`, `--console` / visual per defecte
