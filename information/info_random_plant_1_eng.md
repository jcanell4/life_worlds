([Llegeix en català](info_random_plant_1_cat.md))
([Leer en español](info_random_plant_1_spa.md))

# Simulation `random_plant_1`: plants and learning agents

Configuration: [`config/random_plant_ql_1.json`](../config/random_plant_ql_1.json)

```bash
./build/linux/life_worlds --config config/random_plant_ql_1.json    # Linux
./build/darwin/life_worlds --config config/random_plant_ql_1.json   # macOS
```

## The world

The simulation represents a world (a 24×24 grid) with two types of plants and several agents of the same type that learn by reinforcement with a method called QL (Q-learning).

## The plants

One of the plants, the one drawn as a square, is indigestible and takes energy away from the agents that eat it. The other one (circle) is suitable food for the agents and gives them energy when they eat it.

Plants live for a while and go through different stages. In some of these stages they can reproduce and generate new plants. New plants appear at random in some empty cell.

For each of the two plant types and for each of their stages there is a certain death rate. This way the plants simulate a life cycle in which they are born, grow, die and, if they can, reproduce before dying.

The square plant has a certain reproductive advantage over the round one. If we left them without limits, the first one would end up dominating the whole world.

Through the configuration we can set a minimum number of each plant type and also limit the maximum, to control the training of the agents and the environment they have to live in.

## The agents

The agents (triangle) can move around the world in 4 directions (N/S/E/W) at a rate of one cell per cycle. In every cycle the agent loses a little of its energy. When its energy reaches zero, it dies.

Agents cannot walk through plants, but they can decide to cut them or eat them. If an agent decides to cut a plant, it receives no energy. If it decides to eat a square plant, it loses a significant part of its energy. On the other hand, if it eats a circular plant, it gains a significant part.

Agents observe the world with configurable vision rays (see the next section). Leaving the map ends the episode (`precipice_reward`).

## How the agent sees the world

Configured with `vision_rays` on the agent (`4` or `8`, default 4). Movement actions stay N/S/E/W; Eat/Cut only on orthogonal neighbours.

**Each ray** reports the first hit along that direction (up to `max_vision` cells, or `grid_size` if `max_vision` is `0`):

- **Distance bucket:** `0` = 1 cell; `1` = 2–4; `2` = 5–10; `3` = farther than 10.
- **Object type:** `herb`, `shrub`, another agent, or `empty_edge` (empty cells until the map border / end of range).
- **Growth stage** (float in `[0, 1]`): filled for plants as `(stage_index + 1) / stage_count`; `0` for agents, edge, or non-staged plants. **QL ignores this field** when building its state key.

**Ray order**

- `4` (orthogonal): N, S, W, E.
- `8` (octants): N, NE, E, SE, S, SW, W, NW.

**Energy** is also sensed as one of four buckets: `[75%,100%]`, `[50%,75%)`, `[25%,50%)`, `[0%,25%)`.

**QL encoding:** a compact discrete key from the `ray_count` pairs `(distance, type)` plus the energy bucket (no growth stage). Theoretical size with 4 rays: 262 144 states; with 8 rays the space is much larger (sparse Q-table).

Leaving the grid is a **precipice** (`precipice_reward`, default −20) and ends the episode. Root `survive_cycles` can stop the run when every active agent has survived that many cycles; only applies in `train` (ignored in `eval`).


## Eaten and cut plants

Cut or eaten plants disappear as far as the agents are concerned, but the application shows them so that the user can tell which action the agent chose. Eaten plants are shown in gray with an E (*eaten*) in the middle, and cut plants in brown with a K (*killed*) in the centre. This is information for the user only, which the agents do not perceive: for all purposes, cells holding it are treated as free cells.

## Learning

At first the agents are clueless and act at random, but as they learn they discover that it is better to cut the square plants and eat the round ones. They also discover that cutting a square plant can give them access to a round one, and that they have to move to find food and not die, especially when their energy is at non-optimal levels.

To make learning happen, the agent gives itself a series of rewards and punishments depending on the energy received or spent and on the energy level it is at.

## Configuration and training

In the configuration JSON (`config/random_plant_ql_1.json`) you can change some parameters of the simulation, for both plants and agents. For the agents you can say whether you want them to train (`train`) or to be evaluated (`eval`) with the `mode` attribute, as well as other parameters related to learning, the amount of energy received or spent, and so on.

The agents accumulate their experience in a table that is saved as a binary file called `ql_qtable.bin`. Initially the table does not exist and the agents act at random. Change the `mode` attribute to `"train"` in the agents' definition (`ql_agent`) so that they can train. In training mode, every time an agent dies a new one is born. Since the knowledge table is shared, the new ones inherit all the knowledge of the dead ones.

Run the application for a whole night and you will see that the agents have learned a lot. The table is saved when the application closes (by closing the window or pressing Esc), not before. Once they know enough, you can switch them to `"eval"` mode. In this mode, once dead they are not born again.

Training can be sped up with the F8 / Ctrl+P keys (which stop drawing the world) and by using multiple agents, since in this simulation they all share the table.
