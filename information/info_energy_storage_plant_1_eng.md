([Llegeix en català](info_energy_storage_plant_1_cat.md))
([Leer en español](info_energy_storage_plant_1_spa.md))

# Simulation `energy_storage_plant_1`: plants that store energy

Configuration: [`config/energy_storage_plant_1.json`](../config/energy_storage_plant_1.json)

```bash
./build/linux/life_worlds --config config/energy_storage_plant_1.json    # Linux
./build/darwin/life_worlds --config config/energy_storage_plant_1.json   # macOS
```

## The world

The simulation represents a world (a 24×24 grid) that contains only plants: two types of plants that accumulate energy and, when they have enough, split and occupy a neighbouring cell. There are no agents.

## The plants

One of the types (`moss`) is drawn as a hexagon; the other one (`vine`), as a diamond.

At the start, 3 `moss` plants and 2 `vine` plants appear in empty cells chosen at random. Each of these initial plants is the founder of a lineage: every plant that descends from it belongs to the same lineage.

In every cycle each plant gains a little energy, until it reaches its maximum. `moss` grows faster (5 units per cycle, up to 100) than `vine` (3 units per cycle, up to 80).

When a plant reaches its maximum energy, it splits: it creates a new plant of the same type in one of the free adjacent cells (up, down, left or right), chosen at random. The new plant receives part of the mother's energy and the mother keeps the rest. `moss` shares the energy equally; `vine` gives away 40% and keeps 60%. After that, mother and daughter accumulate energy again until they can split once more.

A plant with no free adjacent cell is stalled: it neither accumulates energy nor splits.

Plants do not die. What limits growth is the size of the lineage: a `moss` lineage can occupy at most 12 cells, and a `vine` lineage, 20. When a lineage reaches its limit, its plants finish filling up with energy and no longer split.

## What you see on screen

The brightness of each plant shows the energy it has stored: dark when it has little and bright when it is full. This lets you see each plant charging up and how, when it splits, mother and daughter suddenly go darker.

All `moss` plants share the same colour (teal), set in the configuration. `vine` has no colour set, so each `vine` lineage is drawn in its own colour: that way you can see how far each one has spread.

The bottom bar shows the current cycle and how many living plants there are of each type.

## How it evolves

During the first cycles nothing seems to happen: the plants are only filling up. The first `moss` split comes at cycle 19, and the first `vine` split at cycle 26. From then on the patches grow faster and faster, because every new plant splits too. `moss` completes its lineages sooner; `vine` takes longer but ends up occupying more space.

Since there are 3 `moss` lineages and 2 `vine` lineages, the world ends up with at most 36 `moss` (3 × 12) and 40 `vine` (2 × 20): 76 plants in a grid of 576 cells. With the seed in the configuration (`rng_seed` 42) that point is reached around cycle 100, about 10 seconds at the configured speed (10 cycles per second).

Shortly afterwards the world is stable and no longer changes: plants with a free cell next to them end up full, and the ones that have been surrounded keep the energy they had.

## Configuration

In the configuration JSON (`config/energy_storage_plant_1.json`) you can change the parameters of each plant type: how many initial plants there are (`initial_count`) and how much energy they start with (`initial_energy`), the energy they gain in every cycle (`growing_rate`), the maximum energy (`maximum_energy_limit`), the maximum number of cells per lineage (`maximum_cell_occupancy`), the share of the energy the new plant receives (`split_fraction`) and the look (`visual`).

For example, if you raise `maximum_cell_occupancy` a lot, the lineages will end up running into each other and fighting over the space. If you change `rng_seed`, the initial plants will appear in other places.

## Limitations

For now these plants do not interact with the other elements: QL agents and the plants of the `random_plant_1` simulation ignore them, which is why they are not yet combined in a single configuration.
