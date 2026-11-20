([Leer en español](info_random_plant_c51_1_spa.md))
([Read in English](info_random_plant_c51_1_eng.md))

# Simulació `random_plant_c51_1`: plantes i agents C51

Configuració: [`config/random_plant_c51_1.json`](../config/random_plant_c51_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_c51_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_c51_1.json   # macOS
```

## El món

És el mateix món de 24×24 que [`random_plant_1`](info_random_plant_1_cat.md): dues espècies de plantes (`herb` / `shrub`) amb el mateix cicle de vida, límits de població i residus E/K. La diferència és l’agent: aquí aprèn amb **C51** (Categorical DQN) en lloc de la taula Q tabular.

## Els agents C51

Els agents (triangle blau) es mouen en 4 direccions, perden energia a cada cicle i poden menjar o tallar plantes colindants, amb les mateixes recompenses que el QL. Alimenten una xarxa LibTorch que estima una **distribució** del retorn per acció. Format de l’observació: vegeu la secció següent.

Tots els agents d’aquest mòdul **comparteixen** la mateixa xarxa i el mateix buffer de replay: el que aprèn un el poden aprofitar els altres.

## Observació / vector de features

Els mateixos sensors que el QL (`vision_rays` de l’agent: 4 o 8; mateixos buckets de distància/tipus/energia). Per a la xarxa, cada raig esdevé:

`one-hot(distància, 4) + one-hot(tipus, 4) + growth_stage (1 float a [0,1])`

i al final `one-hot(energia, 4)`.

| `vision_rays` | Mida del vector |
|---------------|-----------------|
| 4 | 40 |
| 8 | 76 |

`growth_stage` és `(índex_etapa + 1) / stage_count` si es veu una planta; si no, `0`. Canviar el nombre d’etapes d’una espècie no canvia la longitud del vector. Models entrenats amb 4 rajos són incompatibles amb runs de 8.

Ordre dels rajos: 4 → N,S,O,E; 8 → N,NE,E,SE,S,SO,O,NO. Precipici / `survive_cycles`: com el QL (vegeu [`random_plant_1`](info_random_plant_1_cat.md)).

## Entrenament i avaluació

- `mode: "train"`: ε-greedy, actualitzacions des del replay, i quan un agent es mor se’n crea un de nou (reset) que hereta el model compartit. El model es desa a `model_path` en tancar l’aplicació.
- `mode: "eval"`: accions greedy; un agent mort queda inactiu i ja no torna a néixer.

Pots accelerar l’entrenament amb F8 / Ctrl+P. Canvia `reuse_model` a `true` per continuar des d’un fitxer `.pt` existent.

## Configuració

A més dels paràmetres comuns amb el QL (`agent_count`, `energy_*`, `gamma`, `epsilon`, `precipice_reward`, `vision_rays`…), el JSON permet ajustar `n_atoms`, `v_min` / `v_max`, `hidden_sizes`, `learning_rate`, `batch_size`, `replay_capacity`, `learn_start`, `train_every` i `target_update_every`.