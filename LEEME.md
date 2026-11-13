([Llegeix en català](LLEGEIXME.md))
([Read in English](README.md))

# life_worlds

Simulación ECS en C++ con [EnTT](https://github.com/skypjack/entt) y visualización con [raylib](https://www.raylib.com/): un mundo N×N con módulos configurables (`random_plant`, `ql_agent`) desacoplados del grid mediante **ports** y **adapters**, y dibujados con **capas de render** registradas en el composition root.

## Requisitos

- CMake ≥ 3.20
- Compilador C++17 (Apple Clang, Clang o GCC)
- raylib 5.x

macOS (Homebrew, Apple Silicon o Intel):

```bash
brew install cmake raylib
```

Linux (Debian / Ubuntu). El paquete del sistema evita compilar raylib:

```bash
sudo apt install build-essential cmake pkg-config libraylib-dev
```

Si raylib no está instalado, CMake lo descarga con FetchContent (`-DLIFE_WORLDS_FETCH_RAYLIB=OFF` lo desactiva). EnTT y [nlohmann/json](https://github.com/nlohmann/json) siempre se descargan la primera vez (hace falta Internet). Para compilar raylib desde el código fuente en Linux también hacen falta las cabeceras de ventana y OpenGL:

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
cmake --preset darwin && cmake --build --preset darwin   # macOS
cmake --preset linux && cmake --build --preset linux     # Linux
```

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

Configuración alternativa:

```bash
./build/linux/life_worlds --config config/default.json --console
```

En la ventana: celdas vacías en verde claro; cada especie se dibuja con color de paleta según el índice (círculo si el índice es par, cuadrado si es impar); los agentes QL son triángulos naranja. La barra inferior muestra tick y contadores. **F8**, **F12** o **Ctrl+P** activa/desactiva el modo rápido (sin dibujar el mundo; preferid **F8**, porque raylib usa F12 para capturas).

## Arquitectura (ports y plugins)

- **Ports** (`ports/`): contratos sin grid — `OccupancyPort` (`free_cells`) para plantas; `AgentWorldPort` (`observe`, `apply_action`, spawn/reset) para QL.
- **Adapters grid** (`environment/grid/adapters`): implementan los ports delegando en `GridEnvironment`.
- **Render layers** (`environment/grid/layers`): plugins visuales (`draw_grid`, residuos, plantas, agentes QL) registrados con `Environment::add_render_layer`.
- **Composition root** (`app/grid_compose`): `compose_grid_app` crea el grid, cablea ports + capas, y construye las `Entity` con los ports inyectados. Los process de plant/QL **no** conocen `GridEnvironment`.

## Configuración (JSON)

Edita [`config/default.json`](config/default.json). `load_config` parsea la simulación y llama a `compose_grid_app`, que crea el `environment`, registra las capas de dibujo y una `Entity` por cada entrada de `entities` (orden = orden de `step`).

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

### Entidad `ql_agent` (Q-learning tabular)

Hay que ponerla **después** de `random_plant` en `entities`. El agente **elige** la acción vía `AgentWorldPort`; el adapter grid **aplica** el efecto en `GridEnvironment` (move/eat/cut). Acciones: Up/Down/Left/Right/Eat/Cut. Estado: visión en 4 direcciones `(bucket_distancia, tipo)` con tipos `herb=0`, `shrub=1`, `ql_agent=2`, `empty_edge=3` y distancia `0` (1 celda), `1` (2–4), `2` (5–10), `3` (>10), más el nivel de energía `3` ([75%,100%]), `2` ([50%,75%)), `1` ([25%,50%)), `0` ([0%,25%)). Espacio teórico: \((4 \times 4)^4 \times 4 = 262144\) estados. `max_vision` `0` usa `port.default_vision_range()` (en el grid: `grid_size`).

Movimiento: no se puede entrar en una celda con planta ni con otro agente QL (hay que **comer** o **cortar** antes). Eat/Cut solo actúan sobre una planta **colindante** (orden Up/Down/Left/Right si hay más de una); no desde la misma celda.

Después de Eat/Cut queda un residuo en la celda (**gris** = comida, **marrón** = cortada) hasta que nace una planta viva. El HUD/log muestra contadores acumulados `eat_<especie>` y `cut_<especie>`.

| Parámetro | Descripción |
|-----------|-------------|
| `type` | `"ql_agent"` |
| `mode` | `"train"` (ε-greedy + update Q; energía 0 → reset) o `"eval"` (greedy; energía 0 → score e inactivo) |
| `agent_count` | Número de agentes a generar |
| `energy_max` / `energy_drain` | Energía máxima y drenaje por ciclo (por defecto 100 / 5) |
| `herb_energy_delta` / `shrub_energy_delta` | Cambio de energía (y reward de comer) en Eat |
| `ingestion_factor` | Multiplicadores `[herb, shrub]` sobre el delta de energía (por defecto `[1,1]`) |
| `alpha` / `gamma` / `epsilon` | Hiperparámetros Q-learning |
| `max_vision` | Tope de distancia (`0` = `grid_size`) |
| `q_table_path` | Archivo binario de la tabla Q (save en train al cerrar; load en eval o si `reuse_q_table`) |
| `reuse_q_table` | En `train`, si `true` carga la tabla existente y continúa el aprendizaje; si `false` empieza de cero |

Recompensas por ciclo (banda de energía, después del drenaje/comida): `>75%` → +2; `(50,75%]` → −2; `(25,50%]` → −3; `[0,25%]` → −5. El reward de comer es el delta de energía de la acción Eat.

## Estructura

- `ports/` — `OccupancyPort`, `AgentWorldPort` (contratos sin grid)
- `core/` — `Position`/`Age`, `StepContext`, métricas de tick
- `entity/` — interfaz `Entity`
- `environment/` — interfaz `Environment` (`add_render_layer`) + `grid/` (queries, `apply_action`, `adapters/`, `layers/`)
- `plant/random_plant/` — dinámica poblacional + rasgos visuales (ocupación vía port)
- `agent/ql_agent/` — Q-learning (elige acciones; siente/actúa vía port)
- `app/` — composition root grid (`compose_grid_app`: ports + capas + entities)
- `simulation/` — orquestación, config de display, `events/`
- `config_loader.cpp` — JSON de simulación + llamada a `compose_grid_app`
- `main.cpp` — `--config`, `--console` / visual por defecto
