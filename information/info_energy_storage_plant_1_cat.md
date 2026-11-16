([Leer en español](info_energy_storage_plant_1_spa.md))
([Read in English](info_energy_storage_plant_1_eng.md))

# Simulació `energy_storage_plant_1`: plantes que emmagatzemen energia

Configuració: [`config/energy_storage_plant_1.json`](../config/energy_storage_plant_1.json)

```bash
./build/linux/life_worlds --config config/energy_storage_plant_1.json    # Linux
./build/darwin/life_worlds --config config/energy_storage_plant_1.json   # macOS
```

## El món

La simulació representa un món (una graella de 24×24) on només hi ha plantes: dos tipus de plantes que acumulen energia i que, quan en tenen prou, es divideixen i ocupen una cel·la veïna. No hi ha agents.

## Les plantes

Un dels tipus (`moss`) es representa com un hexàgon; l’altre (`vine`), com un rombe.

Al començament apareixen, en cel·les buides triades a l’atzar, 3 plantes `moss` i 2 plantes `vine`. Cada una d’aquestes plantes inicials és la fundadora d’una filiació: totes les plantes que en descendeixin pertanyeran a la mateixa filiació.

A cada cicle, cada planta guanya una mica d’energia, fins a arribar al seu màxim. La `moss` creix més de pressa (5 unitats per cicle, fins a 100) que la `vine` (3 unitats per cicle, fins a 80).

Quan una planta arriba al màxim d’energia, es divideix: crea una planta nova del mateix tipus en una de les cel·les contigües lliures (amunt, avall, esquerra o dreta), triada a l’atzar. La planta nova rep una part de l’energia de la mare i la mare es queda la resta. La `moss` reparteix l’energia a parts iguals; la `vine` en dona el 40 % i se’n queda el 60 %. Després, mare i filla tornen a acumular energia fins que es poden dividir de nou.

Una planta que no té cap cel·la contigua lliure queda aturada: ni acumula energia ni es divideix.

Les plantes no es moren. El que limita el creixement és la mida de la filiació: una filiació de `moss` pot ocupar com a màxim 12 cel·les, i una de `vine`, 20. Quan una filiació arriba al seu límit, les seves plantes s’acaben d’omplir d’energia i ja no es divideixen més.

## El que es veu a la pantalla

La brillantor de cada planta indica l’energia que té emmagatzemada: fosca quan en té poca i brillant quan està plena. Així es veu com cada planta es va carregant i com, en dividir-se, mare i filla s’enfosqueixen de cop.

Totes les `moss` comparteixen el mateix color (verd blavós), definit a la configuració. La `vine` no té cap color definit, i per això cada filiació de `vine` es dibuixa amb un color propi: així es pot veure fins on s’ha estès cadascuna.

La barra inferior mostra el cicle actual i quantes plantes vives hi ha de cada tipus.

## Com evoluciona

Durant els primers cicles sembla que no passi res: les plantes només s’omplen. La primera divisió de les `moss` arriba al cicle 19, i la de les `vine`, al cicle 26. A partir d’aquí les taques creixen cada cop més de pressa, perquè cada planta nova també es divideix. Les `moss` completen abans les seves filiacions; les `vine` triguen més, però acaben ocupant més espai.

Com que hi ha 3 filiacions de `moss` i 2 de `vine`, el món acaba tenint com a màxim 36 `moss` (3 × 12) i 40 `vine` (2 × 20): 76 plantes en una graella de 576 cel·les. Amb la llavor de la configuració (`rng_seed` 42) s’hi arriba cap al cicle 100, uns 10 segons a la velocitat configurada (10 cicles per segon).

Poc després el món queda estable i ja no canvia: les plantes que tenen alguna cel·la lliure al costat acaben plenes, i les que han quedat envoltades es queden amb l’energia que tenien.

## Configuració

En el JSON de configuració (`config/energy_storage_plant_1.json`) es poden canviar els paràmetres de cada tipus de planta: quantes plantes inicials hi ha (`initial_count`) i amb quanta energia comencen (`initial_energy`), l’energia que guanyen a cada cicle (`growing_rate`), l’energia màxima (`maximum_energy_limit`), el nombre màxim de cel·les per filiació (`maximum_cell_occupancy`), la part de l’energia que rep la planta nova (`split_fraction`) i l’aspecte (`visual`).

Per exemple, si augmentes molt `maximum_cell_occupancy`, les filiacions acabaran topant les unes amb les altres i disputant-se l’espai. Si canvies `rng_seed`, les plantes inicials apareixeran en altres llocs.

## Limitacions

De moment aquestes plantes no interactuen amb la resta d’elements: els agents QL i les plantes de la simulació `random_plant_1` les ignoren, i per això encara no es combinen en una mateixa configuració.
