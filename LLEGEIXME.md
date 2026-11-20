([Leer en español](LEEME.md))
([Read in English](README.md))

# life_worlds

Simulació ECS en C++ amb [EnTT](https://github.com/skypjack/entt) i visualització amb [raylib](https://www.raylib.com/): un món N×N amb mòduls configurables (`random_plant`, `energy_storage_plant`, `ql_agent`, `c51_agent`, `dqn_agent`) desacoblats del grid mitjançant **ports** i **adapters**, i dibuixats amb **capes de render** registrades al composition root.

## Requisits

- CMake ≥ 3.20
- Compilador C++17 (Apple Clang, Clang o GCC)
- raylib 5.x
- [LibTorch](https://pytorch.org/get-started/locally/) (n’hi ha prou amb CPU) per a l’agent C51

macOS (Homebrew, Apple Silicon o Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). El paquet del sistema evita compilar raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

Si raylib no està instal·lat, CMake el descarrega amb FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` ho desactiva). Si falta LibTorch, amb `LIFE_WORLDS_FETCH_LIBTORCH=ON` (per defecte) baixa el zip CPU oficial un cop a `~/.local/libtorch-cpu/libtorch`; si la xarxa falla, l’error indica com apuntar CMake a un Torch de Python (`python3 -c "import torch; print(torch.utils.cmake_prefix_path)"`). EnTT i [nlohmann/json](https://github.com/nlohmann/json) sempre es descarreguen la primera vegada (cal Internet). Per compilar raylib des del codi font a Linux també calen les capçaleres de finestra i OpenGL:

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
# Apunta CMAKE_PREFIX_PATH a una instal·lació de LibTorch (zip precompilat o cache de CI).
export CMAKE_PREFIX_PATH="$HOME/.local/libtorch-cpu/libtorch${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

En execució, afegeix el `lib/` de LibTorch a `LD_LIBRARY_PATH` (Linux) o `DYLD_LIBRARY_PATH` (macOS) si l’enllaçador dinàmic no troba `libtorch.so`.

CI: a GitHub Actions (`.github/workflows/build.yml`) compila en Linux, macOS i Windows (raylib/EnTT/json via FetchContent + LibTorch CPU), fa smokes `--console` de QL i C51 i puja **artifacts** (`life_worlds-linux` / `-macos` / `-windows`) amb el binari i els configs d’exemple. Descarrega’ls a **Actions** → run → Artifacts (caduquen als 14 dies).

## Visió, observació i aturada

### Configuració de visió

- `vision_rays` (a cada entitat agent: `ql_agent` / `c51_agent` / `dqn_agent`): `4` (per defecte) o `8`. Entitats agent diferents al mateix run poden barrejar 4 i 8.
  - **4** — ortogonal: N, S, O, E (mateix ordre històric).
  - **8** — octants: N, NE, E, SE, S, SO, O, NO.
- Si `environment.vision_rays` encara hi és, compose falla amb un error clar (mou-lo a l’agent).
- Les accions de moviment segueixen sent només les 4 cardinals; Eat/Cut només en veïns ortogonals.
- `max_vision` (a l’agent): límit de cel·les del raig (`0` = `grid_size`).

Cada raig reporta el **primer** hit: planta `random_plant` (`herb` / `shrub`), un altre agent, o vora/buit (`empty_edge`). També omple `growth_stage` (vegeu més avall).

### Buckets compartits (tots els agents)

| Camp | Valors |
|------|--------|
| Distància (cel·les) | `0` = 1; `1` = 2–4; `2` = 5–10; `3` = >10 |
| Tipus | `herb=0`, `shrub=1`, `agent=2`, `empty_edge=3` |
| Energia | `3` = [75%,100%]; `2` = [50%,75%); `1` = [25%,50%); `0` = [0%,25%) |
| `growth_stage` | float a `[0,1]`: `0` si no hi ha planta; si n’hi ha `(stage_index+1)/stage_count` |

Canviar el nombre d’etapes d’una espècie **no** canvia la mida del vector neural (només el valor de l’escalar).

### QL (estat tabular)

No usa `growth_stage`. Codifica `ray_count` dígits `(distància, tipus)` + bucket d’energia.

- Amb `vision_rays: 4`: \((4 \times 4)^4 \times 4 = 262\,144\) estats teòrics.
- Amb `vision_rays: 8`: \((4 \times 4)^8 \times 4 = 17\,179\,869\,184\) estats teòrics (taula dispersa; creix molt el nombre de claus vistes).

### C51 / DQN (vector de features)

Per cada raig, en ordre: one-hot distància (4) + one-hot tipus (4) + `growth_stage` (1). Al final, one-hot energia (4).

| `vision_rays` | Mida del vector |
|---------------|-----------------|
| 4 | \(4 \times (4+4+1) + 4 = 40\) |
| 8 | \(8 \times (4+4+1) + 4 = 76\) |

Els models `.pt` entrenats amb 4 rajos **no** són compatibles amb 8 (i a l’inrevés). Sí ho són si només canvia el nombre d’etapes de les plantes.

### Precipici i fi d’entrenament

- `precipice_reward` (agent, per defecte `-20`): sortir del grid acaba l’episodi.
- `survive_cycles` (arrel, `0` = off): aturar quan tots els agents actius tenen `EpisodeCycles >= survive_cycles`. Només s’aplica si hi ha algun procés d’agent en `train`; s’ignora en `eval` pur.

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

Sense `--config`, el binari carrega [`config/default.json`](config/default.json). Altres escenaris:

```bash
./build/linux/life_worlds --config config/random_plant_ql_1.json --console
./build/linux/life_worlds --config config/random_plant_c51_1.json --console
./build/linux/life_worlds --config config/random_plant_dqn_1.json --console
```

A la finestra: cel·les buides en verd clar; cada espècie es dibuixa amb color de paleta segons l’índex (cercle si l’índex és parell, quadrat si és senar) i els agents QL són triangles taronja, tret que el config hi defineixi un [`visual`](#aspecte-de-plantes-i-agents-visual). La barra inferior mostra tick i comptadors. **F8** o **Ctrl+P** activa/desactiva el mode ràpid (sense dibuixar el món).

## Arquitectura (ports i plugins)

- **Ports** (`ports/`): contractes sense grid — `OccupancyPort` (`free_cells`) per a plantes; `AgentWorldPort` (`observe`, `apply_action`, spawn/reset) per a QL.
- **Adapters grid** (`environment/grid/adapters`): implementen els ports delegant a `GridEnvironment`.
- **Render layers** (`environment/grid/render_layers`): les maneres de dibuixar que ofereix cada tipus d’entorn, registrades amb `Environment::add_render_layer`. Les del grid dibuixen en cel·les amb passos compartits (`CellCanvas`: forma, etiqueta) i només llegeixen components de `core` (`Position`, `Visual`, `Energy`), de manera que no depenen de cap mòdul: cada entitat tria la seva amb [`visual.render_layer`](#aspecte-de-plantes-i-agents-visual). Un altre tipus d’entorn definiria el seu propi conjunt.
- **Composition root** (`app/grid_compose`): `compose_grid_app` crea el grid, cables ports + capes, i construeix les `Entity` amb els ports injectats. Els process de plant/QL **no** coneixen `GridEnvironment`.
- **Events** (`core/events`): els mòduls informen del que passa emetent un `events::Event` tipat (naixement, mort, eat, cut…) allà on passa; la simulació compta naixements i morts un sol cop per tick a partir d’aquests esdeveniments.

## Configuració (JSON)

Edita [`config/default.json`](config/default.json) (si omets `--config`), o qualsevol escenari sota `config/`. `load_config` parseja la simulació i crida `compose_grid_app`, que crea l’`environment`, registra les capes de dibuix i una `Entity` per cada entrada de `entities` (ordre = ordre de `step`).

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
| `survive_cycles` | Aturar quan tots els agents actius han sobreviscut aquests cicles (`0` = off; s’ignora si tots els agents són `eval`) |
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

Cada espècie de planta (`random_plant`, `energy_storage_plant`) i cada entitat `ql_agent` admet un atribut compost opcional `visual`. Els tres camps són opcionals; el que falti conserva el valor per defecte del seu mòdul.

```json
"visual": { "shape": "circle", "color": [72, 168, 74], "render_layer": "plain" }
```

| Camp | Valors | Per defecte |
|------|--------|-------------|
| `shape` | `"circle"`, `"square"`, `"triangle"`, `"diamond"`, `"hexagon"` | `random_plant`: cercle si l’índex de l’espècie és parell, quadrat si és senar; `energy_storage_plant`: hexàgon; `ql_agent`: triangle |
| `color` | `[r, g, b]` o `[r, g, b, a]` (0-255), o una cadena `"#RRGGBB"` / `"#RRGGBBAA"` | `random_plant`: paleta segons l’índex de l’espècie; `energy_storage_plant`: un color per filiació; `ql_agent`: taronja |
| `render_layer` | `"plain"`, `"level"`, `"value_below"` | `random_plant`: `plain`; `energy_storage_plant`: `level`; `ql_agent`: `value_below` |

`render_layer` tria la capa de dibuix del grid que pinta l’entitat. Les capes no depenen del mòdul, de manera que qualsevol en pot fer servir qualsevol:

| Capa | Què dibuixa |
|------|-------------|
| `plain` | La forma amb el seu color |
| `level` | La forma, més brillant com més plena té l’energia (valor / màxim), amb el valor al centre |
| `value_below` | La forma amb el seu color i el valor de la seva energia a sota |

Una entitat sense energia (les plantes `random_plant`) es dibuixa igual amb totes tres. L’ordre de dibuix és fix: cel·les buides, residus, `plain`, `level`, `value_below`. Un nom de capa desconegut és un error de configuració.

Notes: una planta menjada o tallada deixa un residu amb la **forma** de la seva espècie (gris = menjada, marró = tallada, sigui quin sigui el color de l’espècie); els residus tenen la seva pròpia capa, que no es tria. A `energy_storage_plant`, si es defineix `color`, totes les filiacions d’aquesta espècie el comparteixen.

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
| `plant_types[].visual` | `{ shape, color, render_layer }` opcional, vegeu [Aspecte de plantes i agents](#aspecte-de-plantes-i-agents-visual) |

### Entitat `energy_storage_plant`

Plantes que acumulen energia a cada cicle i es divideixen en una cel·la contigua lliure en omplir-se. Config d’exemple: [`config/energy_storage_plant_1.json`](config/energy_storage_plant_1.json) (`--config config/energy_storage_plant_1.json`).

- A cada cicle una planta guanya `growing_rate` d’energia, fins a `maximum_energy_limit`. Si no té cap cel·la veïna (ortogonal) lliure, no creix ni acumula energia (reprèn si una veïna s’allibera).
- En arribar a l’energia màxima crea una planta nova (mateixa espècie) en una cel·la contigua lliure: la filla rep `split_fraction` de l’energia de la mare i la mare conserva la resta.
- **Filiació**: cada planta inicial obre una filiació i els seus descendents la comparteixen (mateix color a la finestra; amb la capa per defecte, `level`, la brillantor segueix l’energia emmagatzemada i cada planta en mostra el valor; hexàgons per defecte, configurable amb `visual`). Una filiació no pot ocupar més de `maximum_cell_occupancy` cel·les: en assolir-ho, les seves plantes continuen omplint-se fins a l’energia màxima i llavors deixen de créixer.
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
| `plant_types[].visual` | `{ shape, color, render_layer }` opcional |

### Entitat `ql_agent` (Q-learning tabular)

Cal posar-la **després** de `random_plant` a `entities`. L’agent **tria** l’acció via `AgentWorldPort`; l’adapter grid **aplica** l’efecte a `GridEnvironment` (move/eat/cut). Accions: Up/Down/Left/Right/Eat/Cut. L’observació (rajos, buckets, espai d’estats segons `vision_rays`) es descriu a [Visió, observació i aturada](#visió-observació-i-aturada). `max_vision` `0` usa `port.default_vision_range()` (al grid: `grid_size`).

Moviment: no es pot entrar a una cel·la amb planta ni amb un altre agent (cal **menjar** o **tallar** abans). Eat/Cut només actuen sobre una planta **colindant** (ordre Up/Down/Left/Right si n’hi ha més d’una); no des de la mateixa cel·la.

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
| `precipice_reward` | Recompensa en sortir del grid (per defecte −20) |
| `vision_rays` | `4` (ortogonal) o `8` (octants); compartit per tots els agents d’aquest procés (per defecte 4) |
| `max_vision` | Tope de distància (`0` = `grid_size`) |
| `q_table_path` | Fitxer binari de la taula Q (save en train al tancar; load en eval o si `reuse_q_table`) |
| `reuse_q_table` | En `train`, si `true` carrega la taula existent i continua l’aprenentatge; si `false` comença de zero |
| `visual` | `{ shape, color, render_layer }` opcional per a tots els agents d’aquesta entitat |

Recompenses per cicle (banda d’energia, després del drenatge/menjar): `>75%` → −1; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. El reward de menjar és el delta d’energia de l’acció Eat.

### Entitat `c51_agent` (Categorical DQN / C51)

Mateixa interfície de món que `ql_agent` (mateix `AgentWorldPort`, accions, recompenses i cicle train/eval). En lloc d’una Q-table dispersa aprèn una distribució sobre retorns amb un MLP petit (LibTorch), compartit per tots els agents del procés. El vector de features (40 o 76 floats segons `vision_rays`, amb `growth_stage`) es descriu a [Visió, observació i aturada](#visió-observació-i-aturada). Config d’exemple: [`config/random_plant_c51_1.json`](config/random_plant_c51_1.json) (mateixes plantes que `random_plant_1`).

| Paràmetre | Descripció |
|-----------|------------|
| `type` | `"c51_agent"` |
| `mode` | `"train"` (ε-greedy + updates des del replay; energia 0 → reset) o `"eval"` (greedy; energia 0 → inactiu) |
| `agent_count` / `energy_max` / `energy_drain` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` | El mateix significat que a `ql_agent` |
| `n_atoms` | Àtoms de la distribució de retorn (per defecte 51) |
| `v_min` / `v_max` | Suport de la distribució (per defecte −20 / 20) |
| `hidden_sizes` | Amplades de l’MLP (per defecte `[128, 128]`) |
| `learning_rate` / `batch_size` / `replay_capacity` | Adam + replay |
| `learn_start` | Transicions abans del primer update |
| `train_every` / `target_update_every` | Cadència d’update i sync hard del target |
| `model_path` | Fitxer Torch (save en tancar en train; load en eval o si `reuse_model`) |
| `reuse_model` | En `train`, continuar des d’un fitxer existent |
| `visual` | `{ shape, color, render_layer }` opcional (triangle blau per defecte) |


### Entitat `dqn_agent` (Double DQN)

Mateixa interfície de món que `ql_agent` / `c51_agent`. Aprèn valors Q amb Double DQN (LibTorch), xarxa i replay compartits. Features: les mateixes que C51 (vegeu [Visió, observació i aturada](#visió-observació-i-aturada)). Config d’exemple: [`config/random_plant_dqn_1.json`](config/random_plant_dqn_1.json).

| Paràmetre | Descripció |
|-----------|------------|
| `type` | `"dqn_agent"` |
| `mode` | `"train"` o `"eval"` (mateix cicle reset/inactiu que C51) |
| `agent_count` / `energy_*` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` / `precipice_reward` | Com `ql_agent` / `c51_agent` |
| `hidden_sizes` / `learning_rate` / `batch_size` / `replay_capacity` | MLP + replay |
| `learn_start` / `train_every` / `target_update_every` | Cadència d’aprenentatge |
| `model_path` / `reuse_model` | Checkpoint `.pt` |
| `visual` | `{ shape, color, render_layer }` opcional (triangle verd per defecte) |

## Estructura

- `ports/` — `OccupancyPort`, `AgentWorldPort` (contractes sense grid)
- `core/` — components compartits (`Position`, `Age`, `Energy`, `Visual`), `StepContext`, mètriques de tick, `events/` (esdeveniments tipats comuns a tots els mòduls + recompte de naixements/morts)
- `entity/` — interfície `Entity`
- `environment/` — interfície `Environment` (`add_render_layer`) + `grid/` (queries, `apply_action`, `adapters/`, `render_layers/`: una capa per fitxer + `CellCanvas`)
- `plant/components.hpp`, `plant/common.hpp` — component (`PlantTypeIndex`) i helpers (recompte de vives) compartits pels mòduls de plantes
- `plant/random_plant/` — dinàmica poblacional + trets visuals (ocupació via port), residus de plantes menjades/tallades (`residue`)
- `plant/energy_storage_plant/` — plantes que emmagatzemen energia, amb filiacions i límit de cel·les per filiació
- `agent/` — `Agent` / `EpisodeCycles` / `Inactive` i tipus de visió compartits; `ql_agent/` (Q-learning tabular); `c51_agent/` (C51 / LibTorch); `dqn_agent/` (Double DQN)
- `app/` — composition root grid (`compose_grid_app`: ports + capes + entities)
- `simulation/` — orquestració, config de display
- `config_loader.cpp` — JSON de simulació + crida a `compose_grid_app`
- `main.cpp` — `--config`, `--console` / visual per defecte
