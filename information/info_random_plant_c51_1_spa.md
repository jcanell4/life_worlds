([Llegeix en català](info_random_plant_c51_1_cat.md))
([Read in English](info_random_plant_c51_1_eng.md))

# Simulación `random_plant_c51_1`: plantas y agentes C51

Configuración: [`config/random_plant_c51_1.json`](../config/random_plant_c51_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_c51_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_c51_1.json   # macOS
```

## El mundo

Es el mismo mundo 24×24 que [`random_plant_1`](info_random_plant_1_spa.md): dos especies de plantas (`herb` / `shrub`) con el mismo ciclo de vida, límites de población y residuos E/K. La diferencia es el agente: aquí aprende con **C51** (Categorical DQN) en lugar de la tabla Q tabular.

## Los agentes C51

Los agentes (triángulo azul) se mueven en 4 direcciones, pierden energía en cada ciclo y pueden comer o cortar plantas colindantes, con las mismas recompensas que el QL. Alimentan una red LibTorch que estima una **distribución** del retorno por acción. Formato de la observación: véase la sección siguiente.

Todos los agentes de este módulo **comparten** la misma red y el mismo buffer de replay: lo que aprende uno lo pueden aprovechar los demás.

## Observación / vector de features

Los mismos sensores que el QL (`vision_rays` del agente: 4 u 8; mismos buckets de distancia/tipo/energía). Para la red, cada rayo se convierte en:

`one-hot(distancia, 4) + one-hot(tipo, 4) + growth_stage (1 float en [0,1])`

y al final `one-hot(energía, 4)`.

| `vision_rays` | Tamaño del vector |
|---------------|-------------------|
| 4 | 40 |
| 8 | 76 |

`growth_stage` es `(índice_etapa + 1) / stage_count` si se ve una planta; si no, `0`. Cambiar el número de etapas de una especie no cambia la longitud del vector. Modelos entrenados con 4 rayos son incompatibles con runs de 8.

Orden de los rayos: 4 → N,S,O,E; 8 → N,NE,E,SE,S,SO,O,NO. Precipicio / `survive_cycles`: como el QL (ver [`random_plant_1`](info_random_plant_1_spa.md)).

## Entrenamiento y evaluación

- `mode: "train"`: ε-greedy, actualizaciones desde el replay, y cuando un agente muere nace uno nuevo (reset) que hereda el modelo compartido. El modelo se guarda en `model_path` al cerrar la aplicación.
- `mode: "eval"`: acciones greedy; un agente muerto queda inactivo y ya no vuelve a nacer.

Puedes acelerar el entrenamiento con F8 / Ctrl+P. Cambia `reuse_model` a `true` para continuar desde un fichero `.pt` existente.

## Configuración

Además de los parámetros comunes con el QL (`agent_count`, `energy_*`, `gamma`, `epsilon`, `precipice_reward`, `vision_rays`…), el JSON permite ajustar `n_atoms`, `v_min` / `v_max`, `hidden_sizes`, `learning_rate`, `batch_size`, `replay_capacity`, `learn_start`, `train_every` y `target_update_every`.