([Llegeix en català](info_random_plant_dqn_1_cat.md))
([Leer en español](info_random_plant_dqn_1_spa.md))

# Simulation `random_plant_dqn_1`: plants and DQN agents

Configuration: [`config/random_plant_dqn_1.json`](../config/random_plant_dqn_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_dqn_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_dqn_1.json   # macOS
```

## The world

This is the same 24×24 world as [`random_plant_1`](info_random_plant_1_eng.md): two plant species (`herb` / `shrub`) with the same life cycle, population limits and E/K residues. The difference is the agent: here it learns with **Double DQN** instead of the tabular Q-table or C51.

## The DQN agents

The agents (green triangle) move in 4 directions, lose energy every cycle and can eat or cut adjacent plants, with the same rewards as QL. They feed a LibTorch network that estimates the **Q-value** of each action (Double DQN). Observation layout: see the next section.

Every agent of this module **shares** the same network and the same replay buffer. The update is Double DQN: the online net picks the best next action and the target net evaluates it.

## Observation / feature vector

Same sensors as QL (agent `vision_rays` 4 or 8; same distance/type/energy buckets). For the network, each ray becomes:

`one-hot(distance, 4) + one-hot(type, 4) + growth_stage (1 float in [0,1])`

then `one-hot(energy, 4)` at the end.

| `vision_rays` | Feature size |
|---------------|--------------|
| 4 | 40 |
| 8 | 76 |

`growth_stage` is `(stage_index + 1) / stage_count` for a seen plant, else `0`. Changing how many plant stages a species has does not change the vector length. Models trained with 4 rays are incompatible with 8-ray runs.

Ray order: 4 → N,S,W,E; 8 → N,NE,E,SE,S,SW,W,NW. Precipice / `survive_cycles`: same as QL (see [`random_plant_1`](info_random_plant_1_eng.md)).

## Training and evaluation

- `mode: "train"`: ε-greedy, updates from the replay, and when an agent dies a new one is born (reset) that inherits the shared model. The model is saved to `model_path` when the application closes.
- `mode: "eval"`: greedy actions; a dead agent stays inactive and is not born again.

You can speed up training with F8 / Ctrl+P. Set `reuse_model` to `true` to continue from an existing `.pt` file.

## Configuration

Besides the parameters shared with QL (`agent_count`, `energy_*`, `gamma`, `epsilon`, `precipice_reward`, `vision_rays`…), the JSON lets you tune `hidden_sizes`, `learning_rate`, `batch_size`, `replay_capacity`, `learn_start`, `train_every` and `target_update_every`. Models trained with 4 rays are not compatible with 8-ray feature sizes.

