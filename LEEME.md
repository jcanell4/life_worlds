([Llegeix en català](LLEGEIXME.md))
([Read in English](README.md))

# life_worlds

Simulación ECS en C++ con [EnTT](https://github.com/skypjack/entt) y visualización con [raylib](https://www.raylib.com/): un mundo N×N con módulos configurables (`random_plant`, `energy_storage_plant`, `ql_agent`, `c51_agent`, `dqn_agent`) desacoplados del grid mediante **ports** y **adapters**, y dibujados con **capas de render** registradas en el composition root.

## Requisitos

- CMake ≥ 3.20
- Compilador C++17 (Apple Clang, Clang o GCC)
- raylib 5.x
- [LibTorch](https://pytorch.org/get-started/locally/) (CPU basta) para el agente C51

macOS (Homebrew, Apple Silicon o Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). El paquete del sistema evita compilar raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

Si raylib no está instalado, CMake lo descarga con FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` lo desactiva). Si falta LibTorch, con `LIFE_WORLDS_FETCH_LIBTORCH=ON` (por defecto) descarga el zip CPU oficial una vez en `~/.local/libtorch-cpu/libtorch`; si la red falla, el error indica cómo apuntar CMake a un Torch de Python (`python3 -c "import torch; print(torch.utils.cmake_prefix_path)"`). EnTT y [nlohmann/json](https://github.com/nlohmann/json) siempre se descargan la primera vez (hace falta Internet). Para compilar raylib desde el código fuente en Linux también hacen falta las cabeceras de ventana y OpenGL:

```bash
sudo apt install libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libxcursor-dev libxinerama-dev libxkbcommon-dev libwayland-dev libasound2-dev
```

El directorio de build depende del sistema (`build/darwin` en macOS, `build/linux` en Linux) para que las dos máquinas no compartan la misma caché de CMake. La compilación desde el editor es en Debug.

## Compilar

Recompilación limpia en la máquina actual (borra solo `build/darwin` o `build/linux`):

```bash
./rebuild.sh
```

En el editor, la tarea por defecto **CMake: build** configura y compila (en macOS: ⇧⌘B). Depurar o ejecutar con una configuración de `launch.json` compila antes, si hace falta.

Desde la terminal, el preset visible es el del sistema (CMake ≥ 3.21):

```bash
# Apunta CMAKE_PREFIX_PATH a una instalación de LibTorch (zip precompilado o caché de CI).
export CMAKE_PREFIX_PATH="$HOME/.local/libtorch-cpu/libtorch${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

En ejecución, añade el `lib/` de LibTorch a `LD_LIBRARY_PATH` (Linux) o `DYLD_LIBRARY_PATH` (macOS) si el enlazador dinámico no encuentra `libtorch.so`.

CI: GitHub Actions (`.github/workflows/build.yml`) compila en Linux, macOS y Windows (raylib/EnTT/json vía FetchContent + LibTorch CPU), hace smokes `--console` de QL y C51 y sube **artifacts** (`life_worlds-linux` / `-macos` / `-windows`) con el binario y los configs de ejemplo. Descárgalos en **Actions** → run → Artifacts (caducan a los 14 días).

## Visión, observación y parada

### Configuración de visión

- `vision_rays` (en cada entidad agente: `ql_agent` / `c51_agent` / `dqn_agent`): `4` (por defecto) u `8`. Distintas entidades agente en el mismo run pueden mezclar 4 y 8.
  - **4** — ortogonal: N, S, O, E (mismo orden histórico).
  - **8** — octantes: N, NE, E, SE, S, SO, O, NO.
- Si `environment.vision_rays` sigue presente, compose falla con un error claro (muévelo al agente).
- Acciones de movimiento siguen siendo solo las 4 cardinales; Eat/Cut solo en vecinos ortogonales.
- `max_vision` (en el agente): tope de celdas del rayo (`0` = `grid_size`).

Cada rayo reporta el **primer** hit: planta `random_plant` (`herb` / `shrub`), otro agente, o borde/vacío (`empty_edge`). También rellena `growth_stage` (ver abajo).

### Buckets compartidos (todos los agentes)

| Campo | Valores |
|-------|---------|
| Distancia (celdas) | `0` = 1; `1` = 2–4; `2` = 5–10; `3` = >10 |
| Tipo | `herb=0`, `shrub=1`, `agent=2`, `empty_edge=3` |
| Energía | `3` = [75%,100%]; `2` = [50%,75%); `1` = [25%,50%); `0` = [0%,25%) |
| `growth_stage` | float en `[0,1]`: `0` si no hay planta; si hay planta `(stage_index+1)/stage_count` |

Cambiar el número de etapas de una especie **no** cambia el tamaño del vector neural (solo el valor del escalar).

### QL (estado tabular)

No usa `growth_stage`. Codifica `ray_count` dígitos `(distancia, tipo)` + bucket de energía.

- Con `vision_rays: 4`: \((4 \times 4)^4 \times 4 = 262\,144\) estados teóricos.
- Con `vision_rays: 8`: \((4 \times 4)^8 \times 4 = 17\,179\,869\,184\) estados teóricos (tabla dispersa; crece mucho el número de claves vistas).

### C51 / DQN (vector de features)

Por cada rayo, en orden: one-hot distancia (4) + one-hot tipo (4) + `growth_stage` (1). Al final, one-hot energía (4).

| `vision_rays` | Tamaño del vector |
|---------------|-------------------|
| 4 | \(4 \times (4+4+1) + 4 = 40\) |
| 8 | \(8 \times (4+4+1) + 4 = 76\) |

Los modelos `.pt` entrenados con 4 rayos **no** son compatibles con 8 (y al revés). Sí lo son si solo cambia el número de etapas de las plantas.

### Precipicio y fin de entrenamiento

- `precipice_reward` (agente, por defecto `-20`): salir del grid termina el episodio.
- `survive_cycles` (raíz, `0` = off): parar cuando todos los agentes activos tienen `EpisodeCycles >= survive_cycles`. Solo aplica si hay algún proceso de agente en `train`; se ignora en `eval` puro.

## Ejecutar

Hay que ejecutar desde la raíz del proyecto (o pasar una ruta válida a `--config`) para que se encuentre el JSON.

Modo visual (ventana raylib; Esc o cerrar para detener):

```bash
./build/darwin/life_worlds   # macOS
./build/linux/life_worlds    # Linux
```

Modo consola / headless (sin ventana):

```bash
./build/darwin/life_worlds --console
# o:
LIFE_WORLDS_HEADLESS=1 ./build/linux/life_worlds
```

Sin `--config`, el binario carga [`config/default.json`](config/default.json). Otros escenarios:

```bash
./build/linux/life_worlds --config config/random_plant_ql_1.json --console
./build/linux/life_worlds --config config/random_plant_c51_1.json --console
./build/linux/life_worlds --config config/random_plant_dqn_1.json --console
```

En la ventana: celdas vacías en verde claro; cada especie se dibuja con color de paleta según el índice (círculo si el índice es par, cuadrado si es impar) y los agentes QL son triángulos naranja, salvo que el config defina un [`visual`](#aspecto-de-plantas-y-agentes-visual) para ellos. La barra inferior muestra tick y contadores. **F8** o **Ctrl+P** activa/desactiva el modo rápido (sin dibujar el mundo).

## Arquitectura (ports y plugins)

- **Ports** (`ports/`): contratos sin grid — `OccupancyPort` (`free_cells`) para plantas; `AgentWorldPort` (`observe`, `apply_action`, spawn/reset) para QL.
- **Adapters grid** (`environment/grid/adapters`): implementan los ports delegando en `GridEnvironment`.
- **Render layers** (`environment/grid/render_layers`): las formas de dibujar que ofrece cada tipo de entorno, registradas con `Environment::add_render_layer`. Las del grid dibujan en celdas con pasos compartidos (`CellCanvas`: forma, etiqueta) y solo leen componentes de `core` (`Position`, `Visual`, `Energy`), así que no dependen de ningún módulo: cada entidad elige la suya con [`visual.render_layer`](#aspecto-de-plantas-y-agentes-visual). Otro tipo de entorno definiría su propio conjunto.
- **Composition root** (`app/grid_compose`): `compose_grid_app` crea el grid, cablea ports + capas, y construye las `Entity` con los ports inyectados. Los process de plant/QL **no** conocen `GridEnvironment`.
- **Events** (`core/events`): los módulos informan de lo que ocurre emitiendo un `events::Event` tipado (nacimiento, muerte, eat, cut…) allí donde ocurre; la simulación cuenta nacimientos y muertes una sola vez por tick a partir de esos eventos.

## Configuración (JSON)

Edita [`config/default.json`](config/default.json) (si omites `--config`), o cualquier escenario bajo `config/`. `load_config` parsea la simulación y llama a `compose_grid_app`, que crea el `environment`, registra las capas de dibujo y una `Entity` por cada entrada de `entities` (orden = orden de `step`).

### Simulación (raíz)

| Parámetro | Descripción |
|-----------|-------------|
| `cell_size_px` | Píxeles por celda en la ventana |
| `hud_height_px` | Altura en píxeles de la barra inferior |
| `hud_lines` | Número de filas del HUD (≥ 1); las métricas se reparten entre ellas |
| `ticks_per_second` | Ritmo de ticks de simulación con el mundo visible (independiente del FPS) |
| `display_fps` | FPS objetivo mientras se dibuja el mundo (por defecto 60) |
| `fast_display_fps` | Cada cuánto se redibuja el overlay en modo rápido (sin limitar los steps; por defecto 30) |
| `fast_slice_seconds` | Cada cuánto se comprueba el teclado en modo rápido mientras se ejecutan steps (por defecto ~1/60 s) |
| `tick_count` | Límite de ticks (`0` = sin límite en visual; 40 en consola) |
| `survive_cycles` | Parar cuando todos los agentes activos han sobrevivido estos ciclos (`0` = off; se ignora si todos los agentes están en `eval`) |
| `rng_seed` | Semilla del generador aleatorio |
| `log_tick_summary` | Una línea de resumen por tick en la consola |
| `log_events` | Log detallado de eventos tipados |
| `cycles_per_time_unit` | Ciclos por unidad de tiempo (p. ej. un año) |
| `environment` | Mundo (`type: grid`, `grid_size`, …) |
| `entities` | Lista de módulos/`Entity` a ejecutar cada tick |

### Environment `grid`

| Parámetro | Descripción |
|-----------|-------------|
| `type` | `"grid"` |
| `grid_size` | N de la cuadrícula N×N |

### Aspecto de plantas y agentes (`visual`)

Cada especie de planta (`random_plant`, `energy_storage_plant`) y cada entidad `ql_agent` admite un atributo compuesto opcional `visual`. Los tres campos son opcionales; lo que falte conserva el valor por defecto de su módulo.

```json
"visual": { "shape": "circle", "color": [72, 168, 74], "render_layer": "plain" }
```

| Campo | Valores | Por defecto |
|-------|---------|-------------|
| `shape` | `"circle"`, `"square"`, `"triangle"`, `"diamond"`, `"hexagon"` | `random_plant`: círculo si el índice de la especie es par, cuadrado si es impar; `energy_storage_plant`: hexágono; `ql_agent`: triángulo |
| `color` | `[r, g, b]` o `[r, g, b, a]` (0-255), o una cadena `"#RRGGBB"` / `"#RRGGBBAA"` | `random_plant`: paleta según el índice de la especie; `energy_storage_plant`: un color por filiación; `ql_agent`: naranja |
| `render_layer` | `"plain"`, `"level"`, `"value_below"` | `random_plant`: `plain`; `energy_storage_plant`: `level`; `ql_agent`: `value_below` |

`render_layer` elige la capa de dibujo del grid que pinta la entidad. Las capas no dependen del módulo, así que cualquiera puede usar cualquiera:

| Capa | Qué dibuja |
|------|------------|
| `plain` | La forma con su color |
| `level` | La forma, más brillante cuanto más llena está su energía (valor / máximo), con el valor en el centro |
| `value_below` | La forma con su color y el valor de su energía debajo |

Una entidad sin energía (las plantas `random_plant`) se dibuja igual con las tres. El orden de dibujo es fijo: celdas vacías, residuos, `plain`, `level`, `value_below`. Un nombre de capa desconocido es un error de configuración.

Notas: una planta comida o cortada deja un residuo con la **forma** de su especie (gris = comida, marrón = cortada, sea cual sea el color de la especie); los residuos tienen su propia capa, que no se elige. En `energy_storage_plant`, si se define `color`, todas las filiaciones de esa especie lo comparten.

### Entidad `random_plant`

| Parámetro | Descripción |
|-----------|-------------|
| `type` | Debe ser `"random_plant"` |
| `offspring_inheritance_jitter` | Variación aleatoria máxima (±) al heredar fertilidad (`0` = copia exacta) |
| `plant_types[].name` | Nombre de la especie |
| `plant_types[].min_population` | Mínimo de ejemplares; `0` = sin siembra forzada |
| `plant_types[].max_population` | Máximo de ejemplares (reproducción y siembra no lo sobrepasan); `0` = sin límite |
| `plant_types[].stage_count` | Número de etapas (máx. 8) |
| `plant_types[].time_units_by_stage` | Duración de cada etapa en unidades de tiempo (`<= 0` → 1) |
| `plant_types[].death_rate_by_stage` | Probabilidad de muerte por etapa |
| `plant_types[].offspring_rate_by_stage` | Probabilidad de descendencia por etapa (valor inicial) |
| `plant_types[].visual` | `{ shape, color, render_layer }` opcional, ver [Aspecto de plantas y agentes](#aspecto-de-plantas-y-agentes-visual) |

### Entidad `energy_storage_plant`

Plantas que acumulan energía en cada ciclo y se dividen en una celda contigua libre al llenarse. Config de ejemplo: [`config/energy_storage_plant_1.json`](config/energy_storage_plant_1.json) (`--config config/energy_storage_plant_1.json`).

- En cada ciclo una planta gana `growing_rate` de energía, hasta `maximum_energy_limit`. Si no tiene ninguna celda vecina (ortogonal) libre, no crece ni acumula energía (reanuda si una vecina se libera).
- Al llegar a la energía máxima crea una planta nueva (misma especie) en una celda contigua libre: la hija recibe `split_fraction` de la energía de la madre y la madre conserva el resto.
- **Filiación**: cada planta inicial abre una filiación y sus descendientes la comparten (mismo color en la ventana; con la capa por defecto, `level`, el brillo sigue la energía almacenada y cada planta muestra su valor; hexágonos por defecto, configurable con `visual`). Una filiación no puede ocupar más de `maximum_cell_occupancy` celdas: al alcanzarlo, sus plantas siguen llenándose hasta la energía máxima y entonces dejan de crecer.
- Se definen varias especies como en `random_plant`, cada una con su `name`. No hay muerte espontánea.
- **Fase 1:** no interactúa con otras entidades. `random_plant` y `ql_agent` ignoran estas plantas, así que de momento no se combinan en el mismo config (podrían solaparse en una celda).

| Parámetro | Descripción |
|-----------|-------------|
| `type` | Debe ser `"energy_storage_plant"` |
| `plant_types[].name` | Nombre de la especie |
| `plant_types[].initial_count` | Plantas iniciales (cada una, una filiación nueva) |
| `plant_types[].initial_energy` | Energía inicial de cada planta inicial (`0` ≤ valor ≤ `maximum_energy_limit`) |
| `plant_types[].growing_rate` | Energía que se suma por ciclo (`>= 0`) |
| `plant_types[].maximum_energy_limit` | Tope de energía de cada planta (`> 0`) |
| `plant_types[].maximum_cell_occupancy` | Máximo de celdas por filiación (`>= 1`) |
| `plant_types[].split_fraction` | Fracción de la energía de la madre que recibe la nueva planta (`0 < valor < 1`, por defecto `0.5`) |
| `plant_types[].visual` | `{ shape, color, render_layer }` opcional |

### Entidad `ql_agent` (Q-learning tabular)

Hay que ponerla **después** de `random_plant` en `entities`. El agente **elige** la acción vía `AgentWorldPort`; el adapter grid **aplica** el efecto en `GridEnvironment` (move/eat/cut). Acciones: Up/Down/Left/Right/Eat/Cut. La observación (rayos, buckets, espacio de estados según `vision_rays`) se describe en [Visión, observación y parada](#visión-observación-y-parada). `max_vision` `0` usa `port.default_vision_range()` (en el grid: `grid_size`).

Movimiento: no se puede entrar en una celda con planta ni con otro agente (hay que **comer** o **cortar** antes). Eat/Cut solo actúan sobre una planta **colindante** (orden Up/Down/Left/Right si hay más de una); no desde la misma celda.

Después de Eat/Cut queda un residuo en la celda (**gris** = comida, **marrón** = cortada) hasta que nace una planta viva. El HUD/log muestra contadores acumulados `eat_<especie>` y `cut_<especie>`.

| Parámetro | Descripción |
|-----------|-------------|
| `type` | `"ql_agent"` |
| `mode` | `"train"` (ε-greedy + update Q; energía 0 → reset) o `"eval"` (greedy; energía 0 → score e inactivo; al salir imprime el score —ciclos aguantados— de todos los agentes, y la energía restante de los que siguen vivos) |
| `agent_count` | Número de agentes a generar |
| `energy_max` / `energy_drain` | Energía máxima y drenaje por ciclo (por defecto 100 / 5) |
| `herb_energy_delta` / `shrub_energy_delta` | Cambio de energía (y reward de comer) en Eat |
| `ingestion_factor` | Multiplicadores `[herb, shrub]` sobre el delta de energía (por defecto `[1,1]`) |
| `alpha` / `gamma` / `epsilon` | Hiperparámetros Q-learning |
| `precipice_reward` | Recompensa al salir del grid (por defecto −20) |
| `vision_rays` | `4` (ortogonal) u `8` (octantes); compartido por todos los agentes de este proceso (por defecto 4) |
| `max_vision` | Tope de distancia (`0` = `grid_size`) |
| `q_table_path` | Archivo binario de la tabla Q (save en train al cerrar; load en eval o si `reuse_q_table`) |
| `reuse_q_table` | En `train`, si `true` carga la tabla existente y continúa el aprendizaje; si `false` empieza de cero |
| `visual` | `{ shape, color, render_layer }` opcional para todos los agentes de esta entidad |

Recompensas por ciclo (banda de energía, después del drenaje/comida): `>75%` → −1; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. El reward de comer es el delta de energía de la acción Eat.

### Entidad `c51_agent` (Categorical DQN / C51)

Misma interfaz de mundo que `ql_agent` (mismo `AgentWorldPort`, acciones, recompensas y ciclo train/eval). En lugar de una Q-table dispersa aprende una distribución sobre retornos con un MLP pequeño (LibTorch), compartido por todos los agentes del proceso. El vector de features (40 o 76 floats según `vision_rays`, con `growth_stage`) se describe en [Visión, observación y parada](#visión-observación-y-parada). Config de ejemplo: [`config/random_plant_c51_1.json`](config/random_plant_c51_1.json) (mismas plantas que `random_plant_1`).

| Parámetro | Descripción |
|-----------|-------------|
| `type` | `"c51_agent"` |
| `mode` | `"train"` (ε-greedy + updates desde replay; energía 0 → reset) o `"eval"` (greedy; energía 0 → inactivo) |
| `agent_count` / `energy_max` / `energy_drain` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` | Mismo significado que en `ql_agent` |
| `n_atoms` | Átomos de la distribución de retorno (por defecto 51) |
| `v_min` / `v_max` | Soporte de la distribución (por defecto −20 / 20) |
| `hidden_sizes` | Anchos del MLP (por defecto `[128, 128]`) |
| `learning_rate` / `batch_size` / `replay_capacity` | Adam + replay |
| `learn_start` | Transiciones antes del primer update |
| `train_every` / `target_update_every` | Cadencia de update y sync hard del target |
| `model_path` | Archivo Torch (save al cerrar en train; load en eval o si `reuse_model`) |
| `reuse_model` | En `train`, continuar desde un fichero existente |
| `visual` | `{ shape, color, render_layer }` opcional (triángulo azul por defecto) |


### Entidad `dqn_agent` (Double DQN)

Misma interfaz de mundo que `ql_agent` / `c51_agent`. Aprende valores Q con Double DQN (LibTorch), red y replay compartidos. Features: mismas que C51 (ver [Visión, observación y parada](#visión-observación-y-parada)). Config de ejemplo: [`config/random_plant_dqn_1.json`](config/random_plant_dqn_1.json).

| Parámetro | Descripción |
|-----------|-------------|
| `type` | `"dqn_agent"` |
| `mode` | `"train"` o `"eval"` (mismo ciclo reset/inactivo que C51) |
| `agent_count` / `energy_*` / `ingestion_factor` / `gamma` / `epsilon` / `vision_rays` / `max_vision` / `precipice_reward` | Como `ql_agent` / `c51_agent` |
| `hidden_sizes` / `learning_rate` / `batch_size` / `replay_capacity` | MLP + replay |
| `learn_start` / `train_every` / `target_update_every` | Cadencia de aprendizaje |
| `model_path` / `reuse_model` | Checkpoint `.pt` |
| `visual` | `{ shape, color, render_layer }` opcional (triángulo verde por defecto) |

## Estructura

- `ports/` — `OccupancyPort`, `AgentWorldPort` (contratos sin grid)
- `core/` — componentes compartidos (`Position`, `Age`, `Energy`, `Visual`), `StepContext`, métricas de tick, `events/` (eventos tipados comunes a todos los módulos + recuento de nacimientos/muertes)
- `entity/` — interfaz `Entity`
- `environment/` — interfaz `Environment` (`add_render_layer`) + `grid/` (queries, `apply_action`, `adapters/`, `render_layers/`: una capa por fichero + `CellCanvas`)
- `plant/components.hpp`, `plant/common.hpp` — componente (`PlantTypeIndex`) y helpers (recuento de vivas) compartidos por los módulos de plantas
- `plant/random_plant/` — dinámica poblacional + rasgos visuales (ocupación vía port), residuos de plantas comidas/cortadas (`residue`)
- `plant/energy_storage_plant/` — plantas que almacenan energía, con filiaciones y límite de celdas por filiación
- `agent/` — `Agent` / `EpisodeCycles` / `Inactive` y tipos de visión compartidos; `ql_agent/` (Q-learning tabular); `c51_agent/` (C51 / LibTorch); `dqn_agent/` (Double DQN)
- `app/` — composition root grid (`compose_grid_app`: ports + capas + entities)
- `simulation/` — orquestación, config de display
- `config_loader.cpp` — JSON de simulación + llamada a `compose_grid_app`
- `main.cpp` — `--config`, `--console` / visual por defecto
